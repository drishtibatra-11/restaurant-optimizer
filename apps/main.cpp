#include "csv_loader.hpp"
#include "model.hpp"
#include <iomanip>
#include <iostream>
#include <string>

static void printSummary(const Dataset& d) {
    std::cout << "Tables:       " << d.tables.size() << "\n"
              << "Stations:     " << d.stations.size() << "\n"
              << "Dishes:       " << d.dishes.size() << "\n"
              << "Reservations: " << d.reservations.size() << "\n"
              << "Orders:       " << d.orders.size() << "\n";
}

static void printReservations(const Dataset& d, size_t limit) {
    std::cout << std::setw(7) << "id" << std::setw(7) << "party" << std::setw(7)
              << "start" << std::setw(6) << "dur" << std::setw(6) << "zone" << "\n";
    for (size_t i = 0; i < d.reservations.size() && i < limit; ++i) {
        const Reservation& r = d.reservations[i];
        std::cout << std::setw(7) << r.id << std::setw(7) << r.party << std::setw(7)
                  << r.start << std::setw(6) << r.dur << std::setw(6) << r.prefZone << "\n";
    }
    if (d.reservations.size() > limit)
        std::cout << "... and " << d.reservations.size() - limit << " more\n";
}

static void printTables(const Dataset& d, size_t limit) {
    std::cout << std::setw(7) << "id" << std::setw(7) << "seats" << std::setw(7)
              << "zone" << "\n";
    for (size_t i = 0; i < d.tables.size() && i < limit; ++i)
        std::cout << std::setw(7) << d.tables[i].id << std::setw(7) << d.tables[i].seats
                  << std::setw(7) << d.tables[i].zone << "\n";
    if (d.tables.size() > limit)
        std::cout << "... and " << d.tables.size() - limit << " more\n";
}

static bool load(const std::string& dir, Dataset& d) {
    Dataset tmp;
    std::string err;
    if (!loadDataset(dir, tmp, err)) {
        std::cout << "ERROR: " << err << "\n";
        return false;
    }
    d = tmp;
    std::cout << "Loaded '" << dir << "' successfully.\n";
    return true;
}

int main(int argc, char** argv) {
    Dataset d;
    bool loaded = false;
    if (argc > 1) loaded = load(argv[1], d);

    while (true) {
        std::cout << "\n=== Restaurant Optimizer ===\n"
                  << "1. Load dataset\n"
                  << "2. Show summary\n"
                  << "3. Show reservations (first 20)\n"
                  << "4. Show tables (first 20)\n"
                  << "5. Exit\n> ";
        std::string choice;
        if (!std::getline(std::cin, choice)) break;

        if (choice == "1") {
            std::cout << "Dataset folder (e.g. data/small): ";
            std::string dir;
            std::getline(std::cin, dir);
            loaded = load(dir, d) || loaded;
        } else if (choice == "2" || choice == "3" || choice == "4") {
            if (!loaded) { std::cout << "Load a dataset first (option 1).\n"; continue; }
            if (choice == "2") printSummary(d);
            if (choice == "3") printReservations(d, 20);
            if (choice == "4") printTables(d, 20);
        } else if (choice == "5") {
            break;
        } else {
            std::cout << "Invalid choice.\n";
        }
    }
    return 0;
}