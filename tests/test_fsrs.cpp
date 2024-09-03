#include <iostream>
#include <cassert>

#include "FSRS.hpp"

void test_repeat_default_arg();
void test_memo_state();
void test_review_card();
void test_datetime();
void test_card_serialize();

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
    test_memo_state();
    test_datetime();
    test_card_serialize();

    return 0;
}

void test_repeat_default_arg()
{
    std::cout << "--function: test_repeat_default_arg()\n\n";

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

void test_memo_state()
{
    std::cout << "--function: test_memo_state()\n\n";

    FSRS f = FSRS(test_w, std::nullopt, std::nullopt);

    Card card = Card();

    time_t time = std::time(nullptr);
    std::tm tm = *localtime(&time);
    std::optional<std::tm> now = tm;

    std::unordered_map<Rating, SchedulingInfo> scheduling_cards =  f.repeat(card, now);

    std::vector<Rating> ratings = {
	Rating::Again,
	Rating::Good,
	Rating::Good,
	Rating::Good,
	Rating::Good,
	Rating::Good,
    };

    std::vector<int> ivl_history = {0, 0, 1, 3, 8, 21};

    assert(ivl_history.size() == ratings.size());

    for (std::size_t i = 0; i < ratings.size(); ++i) {
	card = scheduling_cards[ratings[i]].card;

	time_t now_t = std::mktime(&now.value());
	now_t += ivl_history[i] * 60 * 60 * 24;
	now = *localtime(&now_t);

	scheduling_cards = f.repeat(card, now);
    }

    std::cout
	<< "Stability: "
	<< std::round(scheduling_cards[Rating::Good].card.stability / 0.0001f) * 0.0001f
	<< " / "
	<< 71.4554f << "\n";

    std::cout
	<< "Difficulty: "
	<< std::round(scheduling_cards[Rating::Good].card.difficulty / 0.0001f) * 0.0001f
	<< " / "
	<< 5.0976f << "\n";

    assert(std::round(scheduling_cards[Rating::Good].card.stability / 0.0001f) * 0.0001f == 71.4554f);
    assert(std::round(scheduling_cards[Rating::Good].card.difficulty / 0.0001f) * 0.0001f == 5.0976f);

    std::cout << std::endl;
}

void test_review_card() {
    std::cout << "--function: test_review_card()\n\n";

    FSRS f = FSRS(test_w, std::nullopt, std::nullopt);
    Card card = Card();
    time_t time = std::time(nullptr);
    std::tm tm = *localtime(&time);

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

    assert(ivl_history_test.size() == ratings.size());

    std::vector<int> ivl_history = std::vector<int>();

    for (Rating rating : ratings) {
	std::pair<Card, ReviewLog> t = f.reviewCard(card, rating, now);
	card = t.first;
	ivl_history.push_back(card.scheduledDays);
	now = card.due;
    }
    
    assert(ivl_history == ivl_history_test);

    std::cout << "Testing intervals after repeatedly reviewing cards\n"; 

    for (std::size_t i = 0; i < ivl_history.size(); ++i) {
        std::cout
	    << "Rating: "
	    << ratings[i]
	    << "\n\tExpected interval: "
	    << ivl_history_test[i]
	    << ",\tActual Interval: "
	    << ivl_history[i] << "\n";
    }

    std::cout << std::endl;
}

void test_datetime()
{
    std::cout << "--function: test_datetime()\n\n";

    FSRS f = FSRS(test_w, std::nullopt, std::nullopt);

    Card card = Card();

    time_t card_due_t = std::mktime(&card.due);

    // New cards should be due immediately after creation
    assert(time(nullptr) >= card_due_t);

    // Repeat a card with rating good before next tests 
    time_t time = std::time(nullptr);
    std::tm tm = *localtime(&time);
    std::optional<std::tm> now = tm;

    std::unordered_map<Rating, SchedulingInfo> scheduling_cards = f.repeat(card, now);
    card = scheduling_cards[Rating::Good].card;
    
    assert(card.lastReview.has_value());

    card_due_t = std::mktime(&card.due);
    time_t card_last_review_t = std::mktime(&card.lastReview.value());

    std::cout
	<< "Card due: "
	<< card.due
	<< ",\tCard last review: "
	<< card.lastReview.value() << "\n";

    assert(card_due_t >= card_last_review_t);

    std::cout << std::endl;
}

void test_card_serialize()
{
    std::cout << "--function: test_card_serialize()\n\n";

    FSRS f = FSRS(test_w, std::nullopt, std::nullopt);

    Card card = Card();

    std::unordered_map<Rating, SchedulingInfo> scheduling_cards = f.repeat(card, std::nullopt);

    card = scheduling_cards[Rating::Easy].card;
    
    std::unordered_map<std::string, std::string> card_map = card.toMap();

    Card card2 = Card::fromMap(card_map);

    std::unordered_map<std::string, std::string> card2_map = card2.toMap();

    for (auto const& p : card_map) {
        if (card2_map.find(p.first) == card2_map.end())
            continue;	

        assert(card2_map[p.first] == p.second);

        std::cout
            << "Card 1 [" << p.first << "] : "
            << p.second
            << "\tCard2 [" << p.first << "] : "
            << card2_map[p.first] << "\n";
    }

    std::cout << std::endl;
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
