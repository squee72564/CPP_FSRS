#ifndef MODELS_HPP
#define MODELS_HPP

#include <vector>
#include <unordered_map>
#include <string>
#include <optional>
#include <ctime>
#include <sstream>
#include <iomanip>

enum State {
    New = 0,
    Learning,
    Review,
    Relearning,
    NumState
};

enum Rating {
    Again = 1,
    Hard,
    Good,
    Easy,
    NumRating
};

class ReviewLog {
public:
    Rating rating;
    int scheduledDays;
    int elapsedDays;
    std::tm review;
    State state;

    ReviewLog() = default;
    ReviewLog(Rating rating,
              int scheduledDays,
              int elapsedDays,
              std::tm review,
              State state);
    ~ReviewLog();
    std::unordered_map<std::string, std::string> toMap() const;
    static ReviewLog fromMap(const std::unordered_map<std::string, std::string>& map);
};

class Card {
public:
    std::tm due;
    float stability;
    float difficulty;
    int elapsedDays;
    int scheduledDays;
    int reps;
    int lapses;
    State state;
    std::optional<std::tm> lastReview;

    Card();
    Card(std::tm due, float stability, float difficulty, int elapsedDays, int scheduledDays, int reps, int lapses, State state, std::optional<std::tm> last_review);
    ~Card();
    std::unordered_map<std::string, std::string> toMap() const;
    static Card fromMap(const std::unordered_map<std::string, std::string>& map);
    std::optional<float> getRetrievability(const std::tm& now) const;
};

struct SchedulingInfo {
    Card card;
    ReviewLog reviewLog;
};

class SchedulingCards{
public:
    Card again;
    Card hard;
    Card good;
    Card easy;

    SchedulingCards(Card card);
    ~SchedulingCards();
    void updateState(const State& state);
    void schedule(std::tm& now, int hardInterval, int goodInterval, int easyInterval);
    std::unordered_map<Rating, SchedulingInfo> recordLog(const Card& card, const std::tm& now) const;
};

class Parameters {
public:
    float requestRetention;
    int maximumInterval;
    std::vector<float> w;

    Parameters(std::optional<std::vector<float>> w, std::optional<float> requestRetention, std::optional<int> maximumInterval);
    ~Parameters();
};

#endif
