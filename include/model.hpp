#pragma once
#include <vector>
#include <string>

// All times are minutes since midnight (18:30 -> 1110).

struct Reservation {
    int id;
    int party;      // number of guests
    int start;      // start time
    int dur;        // duration in minutes
    int prefZone;   // preferred zone (0 = no preference)
    int end() const { return start + dur; }
};

struct Table {
    int id;
    int seats;
    int zone;
};

struct Dish {
    int id;
    int prepTime;   // minutes
    int station;    // kitchen station id
    double cost;
    double profit;
};

struct Order {
    int id;
    int resId;               // which reservation placed it
    std::vector<int> dishes; // dish ids
    int placedAt;            // time the order was placed
};

struct Station {
    int id;
    int capacity;   // dishes it can cook at the same time
};

// Everything loaded from the data files
struct Dataset {
    std::vector<Reservation> reservations;
    std::vector<Table> tables;
    std::vector<Dish> dishes;
    std::vector<Order> orders;
    std::vector<Station> stations;
};