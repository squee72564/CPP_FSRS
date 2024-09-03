#ifndef FSRS_HPP
#define FSRS_HPP

#include <algorithm>
#include <utility>
#include <optional>
#include <ctime>
#include <unordered_map>
#include <vector>
#include <cmath>

#include "models.hpp"

class FSRS {
public:
    Parameters p;    
    float decay;
    float factor;

    FSRS(std::vector<float> w, std::optional<float> requestRetention, std::optional<float> maximumInterval);
    ~FSRS();

    std::pair<Card, ReviewLog> reviewCard(Card card,
                                           const Rating rating,
                                           std::optional<std::tm>& now);

    std::unordered_map<Rating, SchedulingInfo> repeat(Card card,
                                                      std::optional<std::tm> now);

    void initDs(SchedulingCards& s) const;

    void nextDs(SchedulingCards& s,
                 const float lastD,
                 const float lastS,
                 const float retrievability,
                 const State state);

    float initStability(const Rating r) const;
    
    float initDifficulty(const Rating r) const;

    float forgettingCurve(const int elapsedDays, const float stability) const;

    int nextInterval(const float s);

    float nextDifficulty(const float d, const Rating r);

    float shortTermStability(const float stability, const Rating rating);

    float meanReversion(const float init, const float current);

    float nextRecallStability(const float d, const float s, const float r, const Rating rating);

    float nextForgetStability(const float d, const float s, const float r);


};

#endif
