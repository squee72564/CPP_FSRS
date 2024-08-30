#include "models.hpp"

static const std::string timeFmtStr="%Y-%m-%dT%H:%M:%S";

/**
* REVIEW LOG
**/

ReviewLog(Rating r, int sd, int ed, std::tm rev, State s)
    : rating(r), scheduledDays(sd), elapsedDays(ed), review(rev), state(s) {}

ReviewLog::~ReviewLog() {}

std::unordered_map<std::string, std::string> ReviewLog::toMap() const
{
    std::unordered_map<std::string, std::string> ret;

    ret["rating"] = std::to_string(rating);

    ret["scheduledDays"] = std::to_string(scheduledDays);

    ret["elapsedDays"] = std::to_string(elapsedDays);

    std::ostringstream oss;
    oss << std::put_time(&review, timeFmtStr);
    ret["review"] = oss.str();

    ret["state"] = std::to_string(state);

    return ret;
}

static ReviewLog ReviewLog::fromMap(const std::unordered_map<std::string, std::string>& map)
{
    Rating rating = std::static_cast<Rating>(std::stoi(map["rating"]));

    int scheduledDays = std::stoi(map["scheduledDays"]);

    int elapsedDays = std::stoi(map["elapsedDays"]);

    std::tm review = {};
    std::istringstream iss(map["review"]);
    iss >> std::get_time(&review, timeFmtStr);
    
    State state = std::static_cast<State>(std::stoi(map["state"]));
    
    return ReviewLog(rating, scheduledDays, elapsedDays, review, state);
}

/**
* CARD
**/

Card::Card(std::tm d,
           float st,
           float d,
           float ed,
           float sd,
           int r,
           int l,
           State s,
           std::optional<std::tm> lr)
    : due(d), stability(st), difficulty(d), elapsedDays(ed),
      scheduledDays(sd), reps(r), lapses(l), state(s), last_review(lr)
{
    if (std::mktime(&due) == -1) {
        time_t now = time(0);
        due = *gmtime(&now);
    }
}

Card::~Card() {}

std::unordered_map<std::string, std::string> Card::toMap() const
{
    std::unordered_map<std::string, std::string> ret;

    std::ostringstream oss;
    oss << std::put_time(&due, timeFmtStr);
    ret["due"] = oss.str();

    ret["stability"] = std::to_string(stability);
    ret["difficulty"] = std::to_string(difficulty);
    ret["elapsedDays"] = std::to_string(elapsedDays);
    ret["scheduledDays"] = std::to_string(scheduledDays);
    ret["reps"] = std::to_string(reps);
    ret["lapses"] = std::to_string(lapses);
    ret["state"] = std::to_string(state);
    
    if (last_review.has_value()) {
        oss.str("");
        oss.clear();
        oss << std::put_time(&last_review.value(), timeFmtStr);
        ret["lastReview"] = oss.str();
    } else {
        ret["lastReview"] = "N/A";
    }

    return ret;
}

static Card Card::fromMap(const std::unordered_map<std::string, std::string>& map)
{
    std::tm due = {};
    std::istringstream iss(map["due"]);
    iss >> std::get_time(&due, timeFmtStr);

    stability = std::stof(map["stability"]);
    difficulty = std::stof(map["difficulty"]);
    elapsedDays = std::stoi(map["elapsedDays"]);
    scheduledDays = std::stoi(map["scheduledDays"]);
    reps = std::stoi(map["reps"]);
    lapses = std::stoi(map["lapses"]);
    state = std::static_cast<State>(std::stoi(map["state"]));


    lastReview = std::nullopt;
    if (map["review"] != "N/A") {
        std::tm review = {};
        std::istringstream iss(map["review"]);
        iss >> std::get_time(&review.value(), timeFmtStr);
    }

    return Card(due, stability, difficulty, elapsedDays, scheduledDays, reps, lapses, state, lastReview);
}

std::optional<float> Card::getRetrievability(const std::tm& now) const
{
    const float decay = -0.5f;
    const float factor = std::pow(0.9f, 1/decay) - 1;

    if (state == State::Review) {
        time_t now_t = std::mktime(const_cast<std::tm*>(&now));
        time_t last_review_t = std::mktime(const_cast<std::tm*>(&lastReview));
        const seconds_diff = std::difftime(now_t, last_review_t);
        const int days_diff = static_cast<int>(std::floor(seconds_diff / (60 * 60 * 24)));
        return std::pow((1 + factor * elapsed_days / stability), decay);
    }

    return std::nullopt;
}

/**
* SchedulingCards
**/

SchedulingCards::SchedulingCards(Card card)
    : again(card), hard(card), good(card), easy(card) {}

SchedulingCards::~SchedulingCards() {}

void SchedulingCards::updateState(const State& s)
{
    switch (s) {
        case State::New:
            again.state = State::Learning;
            hard.state = State::Learning;
            good.state = State::Learning;
            easy.state = State::Review;
            break;
        case State::Learning:
            again.state = state;
            hard.state = state;
            good.state = State::Review;
            easy.state = State::Review;
            break;
        case State::Review:
            again.state = State::Relearning;
            hard.state = State::Review;
            good.state = State::Review;
            easy.state = State::Review;
            again.lapses += 1;
            break;
    }
}

void SchedulingCards::schedule(const std::tm& now, int hI, int gI, int eI)
{
    again.scheduledDays = 0;
    hard.scheduledDays = hI;
    good.scheduledDays = gI;
    easy.scheduledDays = eI;

    std::time_t now_t = std::mktime(const_cast<std::tm*>(&now));
    std::time_t delta_t = 0;

    delta_t = now_t + 5 * 60;
    again.due = *(std::gmtime(&delta_t));

    if (hI > 0) {
        delta_t = now_t + hI * 60 * 60 * 24;
        hard.due = *(std::gmtime(&delta_t));
    } else {
        delta_t = now_t + 10 * 60;
        hard.due = *(std::gmtime(&delta_t));
    }

    delta_t = now_t + gI * 60 * 60 * 24;
    good.due = *(std::gmtime(delta_t));

    delta_t = now_t + eI * 60 * 60 * 24;
    easy.due = *(std::gmtime(delta_t));
}

std::unordered_map<Rating, SchedulingInfo>
SchedulingCards::recordLog(const Card& card, const std::tm& now) const
{
    return std::unordered_map<Rating, SchedulingInfo> {
        Rating.Again : SchedulingInfo {
            .card = again,
            .reviewLog = ReviewLog(
                Rating.Again,
                again.scheduledDays,
                card.elapsedDays,
                now,
                card.state
            ),
        },
        Rating.Hard : SchedulingInfo {
            .card = hard,
            .reviewLog = ReviewLog(
                Rating.Hard,
                again.scheduledDays,
                card.elapsedDays,
                now,
                card.state
            ),
        },
        Rating.Good : SchedulingInfo {
            .card = good,
            .reviewLog = ReviewLog(
                Rating.Good,
                again.scheduledDays,
                card.elapsedDays,
                now,
                card.state
            ),
        },
        Rating.Easy : SchedulingInfo {
            .card = easy,
            .reviewLog = ReviewLog(
                Rating.Easy,
                again.scheduledDays,
                card.elapsedDays,
                now,
                card.state
            ),
        },
    };
}

/**
* Parameters
**/

Parameters::Parameters(std::optional<std::vector<float>> w, std::optional<float> rr, std::optional<int> mi)
{
    this->w = w.value_or(
        std::vector<float> {
            0.4072,
            1.1829,
            3.1262,
            15.4722,
            7.2102,
            0.5316,
            1.0651,
            0.0234,
            1.616,
            0.1544,
            1.0824,
            1.9813,
            0.0953,
            0.2975,
            2.2042,
            0.2407,
            2.9466,
            0.5034,
            0.6567
        }
    );

    this.requestRetention = rr.value_or(0.9f);

    this.maximumInterval = mi.value_or(36500);
}

Parameters::~Parameters() {}

