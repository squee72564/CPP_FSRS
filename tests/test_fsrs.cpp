#include <gtest/gtest.h>

#include <cmath>
#include <ctime>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <optional>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "FSRS.hpp"

std::ostream& operator<<(std::ostream& os, const Rating r);
std::ostream& operator<<(std::ostream& os, const State s);
std::ostream& operator<<(std::ostream& os, const std::tm& tm);

static std::tm make_utc_tm(int year, int mon, int mday, int hour, int min, int sec)
{
    std::tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = mon - 1;
    tm.tm_mday = mday;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;
    tm.tm_isdst = 0;
    return tm;
}

std::vector<float> test_w = {
    0.4197,
    1.1869,
    3.0412,
    15.2441,
    7.1434,
    0.6477,
    1.0007,
    0.0674,
    1.6597,
    0.1712,
    1.1178,
    2.0225,
    0.0904,
    0.3025,
    2.1214,
    0.2498,
    2.9466,
    0.4891,
    0.6468,
};

TEST(FSRS, RepeatDefaultArg)
{
    FSRS f = FSRS();
    Card card = Card();

    std::unordered_map<Rating, SchedulingInfo> scheduling_cards = f.repeat(card);

    for (int r = Rating::Again; r != Rating::NumRating; r++) {
        Rating card_rating = static_cast<Rating>(r);
        card = scheduling_cards[card_rating].card;

        time_t now = std::time(nullptr);
        std::tm now_gmtime = *std::gmtime(&now);
        time_t due = internal_timegm(&card.due);
        double diff = std::difftime(due, internal_timegm(&now_gmtime));
        (void)diff;
    }
}

TEST(FSRS, MemoState)
{
    FSRS f = FSRS(test_w);
    Card card = Card();

    time_t time = std::time(nullptr);
    std::tm tm = *std::gmtime(&time);
    std::optional<std::tm> now = tm;

    std::unordered_map<Rating, SchedulingInfo> scheduling_cards = f.repeat(card, now);

    std::vector<Rating> ratings = {
        Rating::Again,
        Rating::Good,
        Rating::Good,
        Rating::Good,
        Rating::Good,
        Rating::Good,
    };

    std::vector<int> ivl_history = {0, 0, 1, 3, 8, 21};

    ASSERT_EQ(ivl_history.size(), ratings.size());

    for (std::size_t i = 0; i < ratings.size(); ++i) {
        card = scheduling_cards[ratings[i]].card;

        time_t now_t = internal_timegm(&now.value());
        now_t += ivl_history[i] * 60 * 60 * 24;
        now = *std::gmtime(&now_t);

        scheduling_cards = f.repeat(card, now);
    }

    float stability = std::round(scheduling_cards[Rating::Good].card.stability / 0.0001f) * 0.0001f;
    float difficulty = std::round(scheduling_cards[Rating::Good].card.difficulty / 0.0001f) * 0.0001f;

    EXPECT_FLOAT_EQ(stability, 71.4554f);
    EXPECT_FLOAT_EQ(difficulty, 5.0976f);
}

TEST(FSRS, ReviewCard)
{
    FSRS f = FSRS(test_w, std::nullopt, std::nullopt);
    Card card = Card();
    time_t time = std::time(nullptr);
    std::tm tm = *std::gmtime(&time);

    std::optional<std::tm> now = tm;

    std::vector<Rating> ratings = {
        Rating::Good,
        Rating::Good,
        Rating::Good,
        Rating::Good,
        Rating::Good,
        Rating::Good,
        Rating::Again,
        Rating::Again,
        Rating::Good,
        Rating::Good,
        Rating::Good,
        Rating::Good,
        Rating::Good
    };

    std::vector<int> ivl_history_test = {
        0,
        4,
        17,
        62,
        198,
        563,
        0,
        0,
        9,
        27,
        74,
        190,
        457
    };

    ASSERT_EQ(ivl_history_test.size(), ratings.size());

    std::vector<int> ivl_history;

    for (Rating rating : ratings) {
        std::pair<Card, ReviewLog> t = f.reviewCard(card, rating, now);
        card = t.first;
        ivl_history.push_back(card.scheduledDays);
        now = card.due;
    }

    EXPECT_EQ(ivl_history, ivl_history_test);
}

TEST(FSRS, DateTime)
{
    FSRS f = FSRS();
    Card card = Card();

    time_t card_due_t = internal_timegm(&card.due);
    time_t now_t = std::time(nullptr);
    std::tm now_gmtime = *std::gmtime(&now_t);

    // New cards should be due immediately after creation
    EXPECT_GE(internal_timegm(&now_gmtime), card_due_t);

    // Repeat a card with rating good before next tests
    time_t time = std::time(nullptr);
    std::tm tm = *std::gmtime(&time);
    std::optional<std::tm> now = tm;

    std::unordered_map<Rating, SchedulingInfo> scheduling_cards = f.repeat(card, now);
    card = scheduling_cards[Rating::Good].card;

    EXPECT_TRUE(card.lastReview.has_value());

    card_due_t = internal_timegm(&card.due);
    time_t card_last_review_t = internal_timegm(&card.lastReview.value());

    EXPECT_GE(card_due_t, card_last_review_t);
}

TEST(FSRS, CardSerialize)
{
    FSRS f = FSRS();
    Card card = Card();

    std::unordered_map<Rating, SchedulingInfo> scheduling_cards = f.repeat(card, std::nullopt);

    card = scheduling_cards[Rating::Easy].card;

    std::unordered_map<std::string, std::string> card_map = card.toMap();

    Card card2 = Card::fromMap(card_map);

    std::unordered_map<std::string, std::string> card2_map = card2.toMap();

    for (const auto& [key, val] : card_map) {
        ASSERT_NE(card2_map.find(key), card2_map.end());
        EXPECT_EQ(card2_map[key], val);
    }

    nlohmann::json json1 = card_map;
    nlohmann::json json2 = card2_map;

    EXPECT_EQ(json1, json2);

    std::unordered_map<std::string, std::string> map_check_1 =
        json1.get<std::unordered_map<std::string, std::string>>();
    std::unordered_map<std::string, std::string> map_check_2 =
        json2.get<std::unordered_map<std::string, std::string>>();

    EXPECT_EQ(map_check_1, card_map);
    EXPECT_EQ(map_check_2, card2_map);
}

TEST(FSRS, ReviewLogSerialize)
{
    FSRS f = FSRS();

    Card card = Card();

    // Repeat a card to get the review log
    std::unordered_map<Rating, SchedulingInfo> scheduling_cards = f.repeat(card);

    card = scheduling_cards[Rating::Again].card;

    ReviewLog review_log = scheduling_cards[Rating::Again].reviewLog;

    std::unordered_map<std::string, std::string> review_log_map = review_log.toMap();

    ReviewLog review_log2 = ReviewLog::fromMap(review_log_map);

    std::unordered_map<std::string, std::string> review_log_map2 = review_log2.toMap();

    for (const auto& [key, val] : review_log_map) {
        ASSERT_NE(review_log_map2.find(key), review_log_map2.end());
        EXPECT_EQ(review_log_map2[key], val);
    }

    nlohmann::json json1 = review_log_map;
    nlohmann::json json2 = review_log_map2;

    EXPECT_EQ(json1, json2);

    std::unordered_map<std::string, std::string> map_check_1 =
        json1.get<std::unordered_map<std::string, std::string>>();
    std::unordered_map<std::string, std::string> map_check_2 =
        json2.get<std::unordered_map<std::string, std::string>>();

    EXPECT_EQ(map_check_1, review_log_map);
    EXPECT_EQ(map_check_2, review_log_map2);
}

TEST(FSRS, CustomSchedulerArgs)
{
    FSRS f = FSRS(
        std::vector<float> {
            0.4197f,
            1.1869f,
            3.0412f,
            15.2441f,
            7.1434f,
            0.6477f,
            1.0007f,
            0.0674f,
            1.6597f,
            0.1712f,
            1.1178f,
            2.0225f,
            0.0904f,
            0.3025f,
            2.1214f,
            0.2498f,
            2.9466f,
            0.0f,
            0.6468f,
        },
        0.9f,
        36500
    );

    Card card = Card();

    time_t time = std::time(nullptr);
    std::tm tm = *std::gmtime(&time);
    std::optional<std::tm> now = tm;

    std::vector<Rating> ratings = {
        Rating::Good,
        Rating::Good,
        Rating::Good,
        Rating::Good,
        Rating::Good,
        Rating::Good,
        Rating::Again,
        Rating::Again,
        Rating::Good,
        Rating::Good,
        Rating::Good,
        Rating::Good,
        Rating::Good,
    };

    std::vector<int> ivl_history;

    std::vector<int> test_ivl_history = {
        0,
        3,
        13,
        50,
        163,
        473,
        0,
        0,
        12,
        34,
        91,
        229,
        541
    };

    ASSERT_EQ(ratings.size(), test_ivl_history.size());

    for (Rating rating : ratings) {
        auto [card2, _] = f.reviewCard(card, rating, now);
        card = card2;
        ivl_history.push_back(card.scheduledDays);
        time_t now_t = internal_timegm(&card.due);
        now = *std::gmtime(&now_t);
    }

    for (std::size_t i = 0; i < ivl_history.size(); ++i) {
        (void)i;
    }

    EXPECT_EQ(ivl_history, test_ivl_history);

    // Initialize another scheduler and verify params are properly set
    std::optional<std::vector<float>> w = std::vector<float> {
        0.1456f,
        0.4186f,
        1.1104f,
        4.1315f,
        5.2417f,
        1.3098f,
        0.8975f,
        0.0000f,
        1.5674f,
        0.0567f,
        0.9661f,
        2.0275f,
        0.1592f,
        0.2446f,
        1.5071f,
        0.2272f,
        2.8755f,
        1.234f,
        5.6789f,
    };
    std::optional<float> request_retention = 0.85f;
    std::optional<float> max_interval = 3650;

    FSRS f2 = FSRS(
        w,
        request_retention,
        max_interval
    );

    EXPECT_EQ(f2.p.w, w);
    EXPECT_EQ(f2.p.requestRetention, request_retention);
    EXPECT_EQ(f2.p.maximumInterval, max_interval);
}

TEST(FSRS, DeterministicRepeatWithFixedNow)
{
    FSRS f = FSRS();
    Card card = Card();

    std::tm now_tm = make_utc_tm(2024, 1, 2, 3, 4, 5);
    std::optional<std::tm> now = now_tm;

    std::unordered_map<Rating, SchedulingInfo> a = f.repeat(card, now);
    std::unordered_map<Rating, SchedulingInfo> b = f.repeat(card, now);

    for (int r = Rating::Again; r != Rating::NumRating; r++) {
        Rating rating = static_cast<Rating>(r);
        EXPECT_EQ(a[rating].card.toMap(), b[rating].card.toMap());
        EXPECT_EQ(a[rating].reviewLog.toMap(), b[rating].reviewLog.toMap());
    }
}

TEST(FSRS, NewCardDueOrdering)
{
    FSRS f = FSRS();
    Card card = Card();

    std::tm now_tm = make_utc_tm(2024, 2, 1, 0, 0, 0);
    std::optional<std::tm> now = now_tm;

    std::unordered_map<Rating, SchedulingInfo> s = f.repeat(card, now);

    time_t now_t = internal_timegm(&now.value());
    time_t again_t = internal_timegm(&s[Rating::Again].card.due);
    time_t hard_t = internal_timegm(&s[Rating::Hard].card.due);
    time_t good_t = internal_timegm(&s[Rating::Good].card.due);
    time_t easy_t = internal_timegm(&s[Rating::Easy].card.due);

    EXPECT_EQ(again_t, now_t + 60);
    EXPECT_EQ(hard_t, now_t + 5 * 60);
    EXPECT_EQ(good_t, now_t + 10 * 60);
    EXPECT_GT(easy_t, good_t);
}

TEST(FSRS, LearningHardDueFallsBackToTenMinutes)
{
    FSRS f = FSRS();

    std::tm now_tm = make_utc_tm(2024, 3, 1, 12, 0, 0);
    std::optional<std::tm> now = now_tm;

    Card card = Card();
    card.state = State::Learning;
    card.lastReview = now;
    card.stability = 1.0f;
    card.difficulty = 5.0f;

    std::unordered_map<Rating, SchedulingInfo> s = f.repeat(card, now);

    time_t now_t = internal_timegm(&now.value());
    time_t hard_t = internal_timegm(&s[Rating::Hard].card.due);

    EXPECT_EQ(hard_t, now_t + 10 * 60);
}

TEST(FSRS, ReviewIntervalOrdering)
{
    FSRS f = FSRS();

    std::tm last_tm = make_utc_tm(2024, 1, 1, 0, 0, 0);
    std::tm now_tm = make_utc_tm(2024, 1, 15, 0, 0, 0);
    std::optional<std::tm> now = now_tm;

    Card card = Card();
    card.state = State::Review;
    card.lastReview = last_tm;
    card.stability = 10.0f;
    card.difficulty = 5.0f;

    std::unordered_map<Rating, SchedulingInfo> s = f.repeat(card, now);

    int hard_i = s[Rating::Hard].card.scheduledDays;
    int good_i = s[Rating::Good].card.scheduledDays;
    int easy_i = s[Rating::Easy].card.scheduledDays;

    EXPECT_LE(hard_i, good_i);
    EXPECT_LT(good_i, easy_i);
    EXPECT_GE(easy_i, good_i + 1);
}

TEST(FSRS, StateTransitionsFromNew)
{
    FSRS f = FSRS();
    Card card = Card();

    std::tm now_tm = make_utc_tm(2024, 7, 1, 0, 0, 0);
    std::optional<std::tm> now = now_tm;

    std::unordered_map<Rating, SchedulingInfo> s = f.repeat(card, now);

    EXPECT_EQ(s[Rating::Again].card.state, State::Learning);
    EXPECT_EQ(s[Rating::Hard].card.state, State::Learning);
    EXPECT_EQ(s[Rating::Good].card.state, State::Learning);
    EXPECT_EQ(s[Rating::Easy].card.state, State::Review);
}

TEST(FSRS, StateTransitionsFromReview)
{
    FSRS f = FSRS();

    std::tm last_tm = make_utc_tm(2024, 7, 1, 0, 0, 0);
    std::tm now_tm = make_utc_tm(2024, 7, 10, 0, 0, 0);
    std::optional<std::tm> now = now_tm;

    Card card = Card();
    card.state = State::Review;
    card.lastReview = last_tm;
    card.stability = 10.0f;
    card.difficulty = 5.0f;
    card.lapses = 2;

    std::unordered_map<Rating, SchedulingInfo> s = f.repeat(card, now);

    EXPECT_EQ(s[Rating::Again].card.state, State::Relearning);
    EXPECT_EQ(s[Rating::Hard].card.state, State::Review);
    EXPECT_EQ(s[Rating::Good].card.state, State::Review);
    EXPECT_EQ(s[Rating::Easy].card.state, State::Review);

    EXPECT_EQ(s[Rating::Again].card.lapses, card.lapses + 1);
    EXPECT_EQ(s[Rating::Hard].card.lapses, card.lapses);
    EXPECT_EQ(s[Rating::Good].card.lapses, card.lapses);
    EXPECT_EQ(s[Rating::Easy].card.lapses, card.lapses);
}

TEST(FSRS, ReviewLogScheduledDaysMatchCard)
{
    FSRS f = FSRS();
    Card card = Card();

    std::tm now_tm = make_utc_tm(2024, 4, 1, 0, 0, 0);
    std::optional<std::tm> now = now_tm;

    std::unordered_map<Rating, SchedulingInfo> s = f.repeat(card, now);

    for (int r = Rating::Again; r != Rating::NumRating; r++) {
        Rating rating = static_cast<Rating>(r);
        EXPECT_EQ(s[rating].reviewLog.scheduledDays, s[rating].card.scheduledDays);
    }
}

TEST(FSRS, ReviewLogFieldsMatchExpected)
{
    FSRS f = FSRS();

    std::tm last_tm = make_utc_tm(2024, 8, 1, 0, 0, 0);
    std::tm now_tm = make_utc_tm(2024, 8, 11, 0, 0, 0);
    std::optional<std::tm> now = now_tm;

    Card card = Card();
    card.state = State::Review;
    card.lastReview = last_tm;
    card.stability = 10.0f;
    card.difficulty = 5.0f;

    std::unordered_map<Rating, SchedulingInfo> s = f.repeat(card, now);

    const int expected_elapsed = 10;
    time_t now_t = internal_timegm(&now.value());

    for (int r = Rating::Again; r != Rating::NumRating; r++) {
        Rating rating = static_cast<Rating>(r);
        EXPECT_EQ(s[rating].reviewLog.elapsedDays, expected_elapsed);
        EXPECT_EQ(internal_timegm(&s[rating].reviewLog.review), now_t);
    }
}

TEST(FSRS, ElapsedDaysComputedFromLastReview)
{
    FSRS f = FSRS();

    std::tm last_tm = make_utc_tm(2024, 9, 1, 0, 0, 0);
    std::tm now_tm = make_utc_tm(2024, 9, 6, 0, 0, 0);
    std::optional<std::tm> now = now_tm;

    Card card = Card();
    card.state = State::Review;
    card.lastReview = last_tm;
    card.stability = 10.0f;
    card.difficulty = 5.0f;

    std::unordered_map<Rating, SchedulingInfo> s = f.repeat(card, now);

    EXPECT_EQ(s[Rating::Good].card.elapsedDays, 5);
}

TEST(FSRS, NegativeElapsedClampsToZero)
{
    FSRS f = FSRS();

    std::tm last_tm = make_utc_tm(2024, 10, 10, 0, 0, 0);
    std::tm now_tm = make_utc_tm(2024, 10, 1, 0, 0, 0);
    std::optional<std::tm> now = now_tm;

    Card card = Card();
    card.state = State::Review;
    card.lastReview = last_tm;
    card.stability = 10.0f;
    card.difficulty = 5.0f;

    std::unordered_map<Rating, SchedulingInfo> s = f.repeat(card, now);

    EXPECT_EQ(s[Rating::Good].card.elapsedDays, 0);
    EXPECT_EQ(s[Rating::Good].reviewLog.elapsedDays, 0);
}

TEST(FSRS, CardFromMapThrowsOnInvalidOrMissingData)
{
    Card card = Card();
    std::unordered_map<std::string, std::string> map = card.toMap();

    std::unordered_map<std::string, std::string> missing = map;
    missing.erase("stability");
    EXPECT_THROW(Card::fromMap(missing), std::out_of_range);

    std::unordered_map<std::string, std::string> invalid_due = map;
    invalid_due["due"] = "not-a-date";
    EXPECT_THROW(Card::fromMap(invalid_due), std::invalid_argument);

    std::unordered_map<std::string, std::string> invalid_last_review = map;
    invalid_last_review["lastReview"] = "not-a-date";
    EXPECT_THROW(Card::fromMap(invalid_last_review), std::invalid_argument);
}

TEST(FSRS, ReviewLogFromMapThrowsOnInvalidOrMissingData)
{
    ReviewLog log = ReviewLog(Rating::Good, 1, 2, make_utc_tm(2024, 10, 2, 0, 0, 0), State::Review);
    std::unordered_map<std::string, std::string> map = log.toMap();

    std::unordered_map<std::string, std::string> missing = map;
    missing.erase("rating");
    EXPECT_THROW(ReviewLog::fromMap(missing), std::out_of_range);

    std::unordered_map<std::string, std::string> invalid_review = map;
    invalid_review["review"] = "not-a-date";
    EXPECT_THROW(ReviewLog::fromMap(invalid_review), std::invalid_argument);
}

TEST(FSRS, RetrievabilityNulloptUnlessReview)
{
    std::tm now_tm = make_utc_tm(2024, 5, 1, 0, 0, 0);

    Card card = Card();
    card.state = State::New;
    EXPECT_FALSE(card.getRetrievability(now_tm).has_value());

    card.state = State::Learning;
    EXPECT_FALSE(card.getRetrievability(now_tm).has_value());

    card.state = State::Relearning;
    EXPECT_FALSE(card.getRetrievability(now_tm).has_value());
}

TEST(FSRS, RetrievabilityDecreasesOverTime)
{
    std::tm last_tm = make_utc_tm(2024, 1, 1, 0, 0, 0);
    std::tm now_early = make_utc_tm(2024, 1, 2, 0, 0, 0);
    std::tm now_late = make_utc_tm(2024, 1, 10, 0, 0, 0);

    Card card = Card();
    card.state = State::Review;
    card.lastReview = last_tm;
    card.stability = 10.0f;
    card.difficulty = 5.0f;

    auto r1 = card.getRetrievability(now_early);
    auto r2 = card.getRetrievability(now_late);

    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r2.has_value());
    EXPECT_GT(r1.value(), r2.value());
}

TEST(FSRS, RetrievabilityNulloptOnMissingLastReviewOrInvalidStability)
{
    std::tm now_tm = make_utc_tm(2024, 11, 1, 0, 0, 0);

    Card card = Card();
    card.state = State::Review;
    card.stability = 10.0f;
    card.difficulty = 5.0f;
    card.lastReview = std::nullopt;
    EXPECT_FALSE(card.getRetrievability(now_tm).has_value());

    card.lastReview = make_utc_tm(2024, 10, 1, 0, 0, 0);
    card.stability = 0.0f;
    EXPECT_FALSE(card.getRetrievability(now_tm).has_value());
}

TEST(FSRS, ReviewCardMatchesRepeat)
{
    FSRS f = FSRS();
    Card card = Card();

    std::tm now_tm = make_utc_tm(2024, 6, 1, 0, 0, 0);
    std::optional<std::tm> now = now_tm;

    std::unordered_map<Rating, SchedulingInfo> s = f.repeat(card, now);

    for (int r = Rating::Again; r != Rating::NumRating; r++) {
        Rating rating = static_cast<Rating>(r);
        auto [c, log] = f.reviewCard(card, rating, now);
        EXPECT_EQ(c.toMap(), s[rating].card.toMap());
        EXPECT_EQ(log.toMap(), s[rating].reviewLog.toMap());
    }
}

TEST(FSRS, ParameterClamps)
{
    std::vector<float> w = {
        -1.0f,
        -1.0f,
        -1.0f,
        -1.0f,
        100.0f,
        100.0f,
        1.0f,
        0.5f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
        0.0f,
        0.0f,
    };

    FSRS f = FSRS(w, 0.9f, 7);

    EXPECT_GE(f.initStability(Rating::Again), 0.1f);
    EXPECT_GE(f.initStability(Rating::Hard), 0.1f);
    EXPECT_GE(f.initStability(Rating::Good), 0.1f);
    EXPECT_GE(f.initStability(Rating::Easy), 0.1f);

    float d_again = f.initDifficulty(Rating::Again);
    float d_easy = f.initDifficulty(Rating::Easy);
    EXPECT_GE(d_again, 1.0f);
    EXPECT_LE(d_again, 10.0f);
    EXPECT_GE(d_easy, 1.0f);
    EXPECT_LE(d_easy, 10.0f);

    EXPECT_LE(f.nextInterval(100000.0f), 7);
    EXPECT_GE(f.nextInterval(0.0001f), 1);
}

TEST(FSRS, RequestRetentionMonotonicity)
{
    std::vector<float> w = {
        0.4072f,
        1.1829f,
        3.1262f,
        15.4722f,
        7.2102f,
        0.5316f,
        1.0651f,
        0.0234f,
        1.616f,
        0.1544f,
        1.0824f,
        1.9813f,
        0.0953f,
        0.2975f,
        2.2042f,
        0.2407f,
        2.9466f,
        0.5034f,
        0.6567f
    };

    FSRS f_low = FSRS(w, 0.8f, 36500);
    FSRS f_mid = FSRS(w, 0.9f, 36500);
    FSRS f_high = FSRS(w, 0.95f, 36500);

    const float stability = 10.0f;

    int i_low = f_low.nextInterval(stability);
    int i_mid = f_mid.nextInterval(stability);
    int i_high = f_high.nextInterval(stability);

    EXPECT_GE(i_low, i_mid);
    EXPECT_GE(i_mid, i_high);
}

TEST(FSRS, MaximumIntervalClampAppliedInLearningAndReview)
{
    std::vector<float> w = {
        0.4072f,
        1.1829f,
        3.1262f,
        15.4722f,
        7.2102f,
        0.5316f,
        1.0651f,
        0.0234f,
        1.616f,
        0.1544f,
        1.0824f,
        1.9813f,
        0.0953f,
        0.2975f,
        2.2042f,
        0.2407f,
        2.9466f,
        0.5034f,
        0.6567f
    };

    FSRS f = FSRS(w, 0.9f, 1);

    std::tm last_tm = make_utc_tm(2024, 12, 1, 0, 0, 0);
    std::tm now_tm = make_utc_tm(2024, 12, 10, 0, 0, 0);
    std::optional<std::tm> now = now_tm;

    Card learning = Card();
    learning.state = State::Learning;
    learning.lastReview = last_tm;
    learning.stability = 10.0f;
    learning.difficulty = 5.0f;

    std::unordered_map<Rating, SchedulingInfo> s_learning = f.repeat(learning, now);

    EXPECT_LE(s_learning[Rating::Hard].card.scheduledDays, 1);
    EXPECT_LE(s_learning[Rating::Good].card.scheduledDays, 1);
    EXPECT_LE(s_learning[Rating::Easy].card.scheduledDays, 1);

    Card review = Card();
    review.state = State::Review;
    review.lastReview = last_tm;
    review.stability = 10.0f;
    review.difficulty = 5.0f;

    std::unordered_map<Rating, SchedulingInfo> s_review = f.repeat(review, now);

    EXPECT_LE(s_review[Rating::Hard].card.scheduledDays, 1);
    EXPECT_LE(s_review[Rating::Good].card.scheduledDays, 1);
    EXPECT_LE(s_review[Rating::Easy].card.scheduledDays, 1);
}

std::ostream& operator<<(std::ostream& os, const std::tm& tm)
{
    os << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return os;
}

std::ostream& operator<<(std::ostream& os, const Rating r)
{
    switch (r) {
        case Rating::Again:
            os << "Again";
            break;
        case Rating::Hard:
            os << "Hard";
            break;
        case Rating::Good:
            os << "Good";
            break;
        case Rating::Easy:
            os << "Easy";
            break;
        default:
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, const State s)
{
    switch (s) {
        case State::New:
            os << "New";
            break;
        case State::Learning:
            os << "Learning";
            break;
        case State::Review:
            os << "Review";
            break;
        case State::Relearning:
            os << "Relearning";
            break;
        default:
            break;
    }

    return os;
}
