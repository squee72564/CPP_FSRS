# CPP-FSRS

## Installation
You can clone the repo and use the Makefile to build the tests
```
git clone https://github.com/squee72564/CPP_SRS.git
cd CPP_SRS
make
```

## Quickstart

Import and initialize the FSRS scheduler with default values:

```cpp
#include "FSRS.hpp"

FSRS f = FSRS();
```

Create a new Card object:
```cpp
// all new cards are 'due' immediately upon creation
Card card = Card();
```

Choose a rating and review the card:
```cpp
// you can choose one of the four possible ratings
/*
 * Rating.Again # forget; incorrect response
 * Rating.Hard # recall; correct response recalled with serious difficulty
 * Rating.Good # recall; correct response after a hesitation
 * Rating.Easy # recall; perfect response
*/

Rating rating = Rating::Good;

auto [card, review_log] = f.review_card(card, rating)
```

See when the card is due next
```cpp
#include <ctime>

std::tm due = card.due;
time_t now_t = std::time(nullptr);
std::tm now = *std::gmtime(&now_t); // Convert to GMT

// how much time between when the card is due and now
// internal_timegm is used to transform the std::tm that was set with std::gmtime into time_t
int time_delta = std::difftime(internal_timegm(&due) - internal_timegm(&now));
```

## Usage

### Custom scheduler

You can initialize the FSRS scheduler with your own custom weights as well as desired retention rate and maximum interval.

```cpp
#include <vector>

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
        0.4891f,
        0.6468f,
     }, // weights
    0.85, //request_retention
    3650, //maximum_interval 
);
```

### Advanced reviewing of cards

Aside from using the convenience method `reviewCard`, there is also the `repeat` method:

```cpp
#include <ctime>
#include <optional>

// custom review time (will be considered UTC)
std::tm t = {};
t.tm_sec = 56;
t.tm_min = 7;
t.tm_hour = 20;
t.tm_,day = 13;
t.tm_mon = 7;
t.tm_year = 2024 - 1900; // years since 1900

// Make optional to match repeat function signature
std::optional<std::tm> review_time = t;

std::unordered_map<Rating, SchedulingInfo> scheduling_cards = f.repeat(card, review_time);

// can get updated cards for each possible rating
Card card_Again = scheduling_cards[Rating::Again].card;
Card card_Hard = scheduling_cards[Rating::Hard].card;
Card card_Good = scheduling_cards[Rating::Good].card;
Card card_Easy = scheduling_cards[Rating::Easy].card;

// get next review interval for each rating
int scheduled_days_Again = card_Again.scheduled_days;
int scheduled_days_Hard = card_Hard.scheduled_days;
int scheduled_days_Good = card_Good.scheduled_days;
int scheduled_days_Easy = card_Easy.scheduled_days;

// choose a rating and update the card
Rating rating = Rating::Good;
Card card = scheduling_cards[rating].card;

// get the corresponding review log for the review
ReviewLog review_log = scheduling_cards[rating].review_log;
```

### Serialization
`Card` and `ReviewLog` objects are convertible to an std::unordered_map<std::string ,std::string> via their `toMap` and `fromMap` methods.

Once `Card` and `ReviewLog` objects are in this form you can easily JSON-serializable them via the `unorederedMapToJson` method for easy database storage:

```cpp
// serialize before storage
std::unordered_map<std::string ,std::string> card_map = card.toMap();
std::unordered_map<std::string ,std::string> review_log_map = review_log.toMap();

// deserialize from dict
Card new_card = Card::fromMap(card_map);
Card new_review_log = ReviewLog::fromMap(review_log_map);
```

## Reference

Card objects have one of four possible states
```cpp
enum State {
	State::New = 0,    // Never been studied
	State::Learning,   // Been studied for the first time recently
	State::Review,     // Graduate from learning state
	State::Relearning, // Forgotten in review state
};
```

There are four possible ratings when reviewing a card object:
```cpp
enum Rating {
	Rating::Again  = 1, // forget; incorrect response
	Rating::Hard,   	// recall; correct response recalled with serious difficulty
	Rating::Good, 	    // recall; correct response after a hesitation
	Rating::Easy, 	    // recall; perfect response
}
```
