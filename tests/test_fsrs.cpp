#include <iostream>
#include <cassert>

#include "FSRS.hpp"

void test_review_card();
void test_repeat_default_arg();
std::ostream& operator<<(std::ostream& os, const Rating r);
std::ostream& operator<<(std::ostream& os, const State s);
std::ostream& operator<<(std::ostream& os, const std::tm& tm);

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

int main() {
    
    test_repeat_default_arg();
    test_review_card();

    return 0;
}

void test_repeat_default_arg()
{
    FSRS f = FSRS(test_w, std::nullopt, std::nullopt);

    Card card = Card();

    std::unordered_map<Rating, SchedulingInfo> scheduling_cards = f.repeat(card, std::nullopt);

    for (int r = Rating::Again; r != Rating::NumRating; r++) {
        
        Rating card_rating = static_cast<Rating>(r);

        card = scheduling_cards[card_rating].card;

        std::cout
            << "Card at rating " << card_rating << "\n"
            << "Stability: " << card.stability << "\n"
            << "Difficulty: " << card.difficulty << "\n"
            << "Elapsed Days: " << card.elapsedDays << "\n"
            << "Scheduled Days: " << card.scheduledDays << "\n"
            << "Reps: " << card.reps << "\n"
            << "Lapses: " << card.lapses << "\n"
            << "State: " << card.state << "\n" 
            << "Due: " << card.due << "\n";

        if (card.lastReview.has_value()) {
            std::cout << "Last Review: " << card.lastReview.value() << "\n";
        }

        time_t now = std::time(nullptr);
        time_t due = std::mktime(&card.due);
        double diff = std::difftime(due, now);
        
        std::cout << "Time delta (s): " << diff << "\n" << std::endl;
    }

}

void test_review_card() {
    FSRS f = FSRS(test_w, std::nullopt, std::nullopt);
    Card card = Card();
    time_t time = std::time(nullptr);
    std::tm tm = *gmtime(&time);

    std::optional<std::tm> now = tm;
    
    
    std::vector<Rating> ratings = {
        Rating::Good,
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

    std::vector<int> ivl_history = std::vector<int>();

    for (Rating rating : ratings) {
        std::pair<Card, ReviewLog> t = f.reviewCard(Card(), rating, now);
        ivl_history.push_back(t.first.scheduledDays);
        now = card.due;
    }
    
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

    for (int i = 0; i < ivl_history.size(); ++i) {
        std::cout << "t: " << ivl_history_test[i] << " / i: " << ivl_history[i] << "\n";
    }

    assert(ivl_history == ivl_history_test);
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
