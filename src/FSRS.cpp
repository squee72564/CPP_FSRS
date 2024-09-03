#include "FSRS.hpp"

FSRS::FSRS(std::vector<float> w, std::optional<float> requestRetention, std::optional<float> maximumInterval)
    : p{Parameters(w, requestRetention, maximumInterval)}
{
    decay = -0.5f;
    factor = std::pow(0.9f, 1.0f / decay) - 1;
}

FSRS::~FSRS()
{

}

std::pair<Card, ReviewLog> FSRS::reviewCard(Card card, const Rating rating, std::optional<std::tm>& now)
{
    std::unordered_map<Rating, SchedulingInfo> schedulingCards = repeat(card, now);

    Card c = schedulingCards[rating].card;
    ReviewLog r = schedulingCards[rating].reviewLog;

    return std::pair<Card, ReviewLog>{c, r};
}

std::unordered_map<Rating, SchedulingInfo> FSRS::repeat(Card card,
                                                        std::optional<std::tm> now)
{
    if (!now.has_value()) {
        auto curr_time_point = std::chrono::system_clock::now();
        std::time_t tt = std::chrono::system_clock::to_time_t(curr_time_point);
        std::tm tm = *std::localtime(&tt);
        now = tm;
    }

    std::time_t now_t = std::mktime(&now.value());
    std::time_t delta_t = 0;

    if (card.state == State::New) {
        card.elapsedDays = 0;
    } else {
        std::time_t last_review_t = std::mktime(&card.lastReview.value());
        card.elapsedDays = std::difftime(now_t, last_review_t) / (60.0f * 60.0f * 24.0f);
    }

    card.lastReview = now;
    card.reps += 1;

    SchedulingCards s = SchedulingCards(card);
    s.updateState(card.state);

    if  (card.state == State::New) {
        initDs(s);

        delta_t = now_t + 1 * 60;
        s.again.due = *std::localtime(&delta_t);

        delta_t = now_t + 5 * 60;
        s.hard.due = *std::localtime(&delta_t);

        delta_t = now_t + 10 * 60;
        s.good.due = *std::localtime(&delta_t);

        const int easy_interval = nextInterval(s.easy.stability);
        s.easy.scheduledDays = easy_interval;

        delta_t = now_t + easy_interval * 60 * 60 * 24;
        s.easy.due = *std::localtime(&delta_t);

    } else if (card.state == State::Learning || card.state == State::Relearning) {
        const int interval = card.elapsedDays;
        const float last_d = card.difficulty;
        const float last_s = card.stability;
        const float retrieveability = forgettingCurve(interval, last_s);
        nextDs(s, last_d, last_s, retrieveability, card.state);

        const int hard_interval = 0;
        const int good_interval = nextInterval(s.good.stability);
        const int easy_interval = std::max(nextInterval(s.easy.stability), good_interval + 1);
        s.schedule(now.value(), hard_interval, good_interval, easy_interval);
    } else {
        const int interval = card.elapsedDays;
        const float last_d = card.difficulty;
        const float last_s = card.stability;
        const float retrieveability = forgettingCurve(interval, last_s);
        nextDs(s, last_d, last_s, retrieveability, card.state);

        int hard_interval = nextInterval(s.hard.stability);
        int good_interval = nextInterval(s.good.stability);
        hard_interval = std::min(hard_interval, good_interval);
        good_interval = std::max(good_interval, hard_interval + 1);
        int easy_interval = std::max(nextInterval(s.easy.stability), good_interval + 1);
        s.schedule(now.value(), hard_interval, good_interval, easy_interval);
    }

    return s.recordLog(card, now.value());
}

void FSRS::initDs(SchedulingCards& s) const
{
    s.again.difficulty = initDifficulty(Rating::Again);
    s.again.stability = initStability(Rating::Again);
    s.hard.difficulty = initDifficulty(Rating::Hard);
    s.hard.stability = initStability(Rating::Hard);
    s.good.difficulty = initDifficulty(Rating::Good);
    s.good.stability = initStability(Rating::Good);
    s.easy.difficulty = initDifficulty(Rating::Easy);
    s.easy.stability = initStability(Rating::Easy);
}

void FSRS::nextDs(SchedulingCards& s,
                  const float last_d,
                  const float last_s,
                  const float retrieveability,
                  const State state)
{
    s.again.difficulty = nextDifficulty(last_d, Rating::Again);
    s.hard.difficulty = nextDifficulty(last_d, Rating::Hard);
    s.good.difficulty = nextDifficulty(last_d, Rating::Good);
    s.easy.difficulty = nextDifficulty(last_d, Rating::Easy);

    if (state == State::Learning || state == State::Relearning) {
        s.again.stability = shortTermStability(last_s, Rating::Again);
        s.hard.stability = shortTermStability(last_s, Rating::Hard);
        s.good.stability = shortTermStability(last_s, Rating::Good);
        s.easy.stability = shortTermStability(last_s, Rating::Easy);
    } else if (state == State::Review) {
        s.again.stability = nextForgetStability(last_d, last_s, retrieveability);
        s.hard.stability = nextRecallStability(last_d, last_s, retrieveability, Rating::Hard);
        s.good.stability = nextRecallStability(last_d, last_s, retrieveability, Rating::Good);
        s.easy.stability = nextRecallStability(last_d, last_s, retrieveability, Rating::Easy);
    }
}

float FSRS::initStability(const Rating r) const
{
    return std::max(p.w[r-1], 0.1f);
}

float FSRS::initDifficulty(const Rating r) const
{
    return std::min(std::max(p.w[4] - std::expf(p.w[5] * (r-1)) + 1.0f, 1.0f), 10.0f);
}

float FSRS::forgettingCurve(const int elapsedDays, const float stability) const
{
    return std::powf((1 + factor * elapsedDays / stability), decay);
}

int FSRS::nextInterval(const float s)
{
    float new_interval =
        s
        / factor
        * (std::powf(p.requestRetention, 1.0f / decay)-1);

        const int mx = std::max(round(new_interval), 1.0f);
        return std::min(mx, p.maximumInterval);
}

float FSRS::nextDifficulty(const float d, const Rating r)
{
    float next_d = d - p.w[6] * (r-3);

    return std::min(std::max(meanReversion(initDifficulty(Rating::Easy), next_d), 1.0f), 10.0f);
}

float FSRS::shortTermStability(const float stability, const Rating rating)
{
    return stability * std::expf(p.w[17] * (rating - 3 + p.w[18]));
}

float FSRS::meanReversion(const float init, const float current)
{
    return p.w[7] * init + (1 - p.w[7]) * current;
}

float FSRS::nextRecallStability(const float d, const float s,  const float r, const Rating rating)
{
    float hard_penalty = (rating == Rating::Hard) ? p.w[15] : 1.0f;
    float easy_bonus = (rating == Rating::Easy) ? p.w[16] : 1.0f;

    return s * (
          1
        + std::expf(p.w[8])
        * (11-d)
        * std::powf(s, -p.w[9])
        * (std::expf((1-r) * p.w[10]) -1)
        * hard_penalty
        * easy_bonus
    );
}

float FSRS::nextForgetStability(const float d, const float s, const float r)
{
    return (
         p.w[11]
         * std::powf(d, -p.w[12])
         * (std::powf(s + 1, p.w[13]) - 1)
         * std::expf((1 - r) * p.w[14])
    );
}
