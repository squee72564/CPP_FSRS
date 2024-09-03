#include "models.hpp"

static const std::string timeFmtStr="%Y-%m-%dT%H:%M:%S";

/**
* REVIEW LOG
**/

ReviewLog::ReviewLog(Rating r, int sd, int ed, std::tm rev, State s)
    : rating(r), scheduledDays(sd), elapsedDays(ed), review(rev), state(s) {}

ReviewLog::~ReviewLog() {}

std::unordered_map<std::string, std::string> ReviewLog::toMap() const
{
    std::unordered_map<std::string, std::string> ret;

    ret["rating"] = std::to_string(rating);

    ret["scheduledDays"] = std::to_string(scheduledDays);

    ret["elapsedDays"] = std::to_string(elapsedDays);

    std::ostringstream oss;
    oss << std::put_time(&review, timeFmtStr.c_str());
    ret["review"] = oss.str();

    ret["state"] = std::to_string(state);

    return ret;
}

ReviewLog ReviewLog::fromMap(const std::unordered_map<std::string, std::string>& map)
{
    Rating rating = static_cast<Rating>(std::stoi(map.at("rating")));

    int scheduledDays = std::stoi(map.at("scheduledDays"));

    int elapsedDays = std::stoi(map.at("elapsedDays"));

    std::tm review = {};
    std::istringstream iss(map.at("review"));
    iss >> std::get_time(&review, timeFmtStr.c_str());
    
    State state = static_cast<State>(std::stoi(map.at("state")));
    
    return ReviewLog(rating, scheduledDays, elapsedDays, review, state);
}

/**
* CARD
**/

Card::Card()
    : due(std::tm{}), stability(0), difficulty(0), elapsedDays(0), scheduledDays(0),
      reps(0), lapses(0), state(State::New), lastReview(std::nullopt)
{
    std::time_t t = time(nullptr);
    due = *localtime(&t);
}

Card::Card(std::tm due, float st, float d, int ed, int sd, int r, int l, State s, std::optional<std::tm> lr)
    : due(due), stability(st), difficulty(d), elapsedDays(ed),
      scheduledDays(sd), reps(r), lapses(l), state(s), lastReview(lr)
{
    if (std::mktime(&due) == -1) {
        time_t now = time(0);
        due = *localtime(&now);
    }
}

Card::~Card() {}

std::unordered_map<std::string, std::string> Card::toMap() const
{
    std::unordered_map<std::string, std::string> ret;

    std::ostringstream oss;
    oss << std::put_time(&due, timeFmtStr.c_str());
    ret["due"] = oss.str();

    ret["stability"] = std::to_string(stability);
    ret["difficulty"] = std::to_string(difficulty);
    ret["elapsedDays"] = std::to_string(elapsedDays);
    ret["scheduledDays"] = std::to_string(scheduledDays);
    ret["reps"] = std::to_string(reps);
    ret["lapses"] = std::to_string(lapses);
    ret["state"] = std::to_string(state);
    
    if (lastReview.has_value()) {
        oss.str("");
        oss.clear();
        oss << std::put_time(&lastReview.value(), timeFmtStr.c_str());
        ret["lastReview"] = oss.str();
    } else {
        ret["lastReview"] = "N/A";
    }

    return ret;
}

Card Card::fromMap(const std::unordered_map<std::string, std::string>& map)
{
    std::tm due = {};
    std::istringstream iss(map.at("due"));
    iss >> std::get_time(&due, timeFmtStr.c_str());

    const float stability = std::stof(map.at("stability"));
    const float difficulty = std::stof(map.at("difficulty"));
    const int elapsedDays = std::stoi(map.at("elapsedDays"));
    const int scheduledDays = std::stoi(map.at("scheduledDays"));
    const int reps = std::stoi(map.at("reps"));
    const int lapses = std::stoi(map.at("lapses"));
    const State state = static_cast<State>(std::stoi(map.at("state")));


    std::optional<std::tm> lastReview = std::nullopt;
    if (map.at("review") != "N/A") {
        std::istringstream iss(map.at("review"));
        iss >> std::get_time(&lastReview.value(), timeFmtStr.c_str());
    }

    return Card(due, stability, difficulty, elapsedDays, scheduledDays, reps, lapses, state, lastReview);
}

std::optional<float> Card::getRetrievability(const std::tm& now) const
{
    const float decay = -0.5f;
    const float factor = std::pow(0.9f, 1/decay) - 1;

    if (state == State::Review) {
        time_t now_t = std::mktime(const_cast<std::tm*>(&now));
        time_t last_review_t = std::mktime(const_cast<std::tm*>(&lastReview.value()));
        const int seconds_diff = std::difftime(now_t, last_review_t);
        const int days_diff = static_cast<int>(std::floor(static_cast<float>(seconds_diff) / (60.0f * 60.0f * 24.0f)));
        return std::pow((1 + factor * days_diff / stability), decay);
    }

    return std::nullopt;
}

/**
* SchedulingCards
**/

SchedulingCards::SchedulingCards(Card card)
    : again(card), hard(card), good(card), easy(card) {}

SchedulingCards::~SchedulingCards() {}

void SchedulingCards::updateState(const State& state)
{
    switch (state) {
        case State::New:
            again.state = State::Learning;
            hard.state = State::Learning;
            good.state = State::Learning;
            easy.state = State::Review;
            break;
        case State::Learning:
        case State::Relearning:
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
        default:
            break;
    }
}

void SchedulingCards::schedule(std::tm& now, int hI, int gI, int eI)
{
    again.scheduledDays = 0;
    hard.scheduledDays = hI;
    good.scheduledDays = gI;
    easy.scheduledDays = eI;

    std::time_t now_t = std::mktime(const_cast<std::tm*>(&now));
    std::time_t delta_t = 0;

    delta_t = now_t + 5 * 60;
    again.due = *(std::localtime(&delta_t));

    if (hI > 0) {
        delta_t = now_t + hI * 60 * 60 * 24;
        hard.due = *(std::localtime(&delta_t));
    } else {
        delta_t = now_t + 10 * 60;
        hard.due = *(std::localtime(&delta_t));
    }

    delta_t = now_t + gI * 60 * 60 * 24;
    good.due = *(std::localtime(&delta_t));

    delta_t = now_t + eI * 60 * 60 * 24;
    easy.due = *(std::localtime(&delta_t));
}

std::unordered_map<Rating, SchedulingInfo>
SchedulingCards::recordLog(const Card& card, const std::tm& now) const
{
    return std::unordered_map<Rating, SchedulingInfo> {
        {Rating::Again, SchedulingInfo {
            .card = again,
            .reviewLog = ReviewLog(
                Rating::Again,
                again.scheduledDays,
                card.elapsedDays,
                now,
                card.state
            ),
        }},
        {Rating::Hard, SchedulingInfo {
            .card = hard,
            .reviewLog = ReviewLog(
                Rating::Hard,
                again.scheduledDays,
                card.elapsedDays,
                now,
                card.state
            ),
        }},
        {Rating::Good, SchedulingInfo {
            .card = good,
            .reviewLog = ReviewLog(
                Rating::Good,
                again.scheduledDays,
                card.elapsedDays,
                now,
                card.state
            ),
        }},
        {Rating::Easy, SchedulingInfo {
            .card = easy,
            .reviewLog = ReviewLog(
                Rating::Easy,
                again.scheduledDays,
                card.elapsedDays,
                now,
                card.state
            ),
        }},
    };
}

/**
* Parameters
**/

Parameters::Parameters(std::optional<std::vector<float>> weights, std::optional<float> rr, std::optional<int> mi)
{
    w = weights.value_or(
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

    requestRetention = rr.value_or(0.9f);

    maximumInterval = mi.value_or(36500);
}

Parameters::~Parameters() {}

