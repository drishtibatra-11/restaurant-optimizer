#include "csv_loader.hpp"
#include "index.hpp"
#include "test.hpp"
#include <algorithm>
#include <random>
#include <string>
#include <vector>

static std::vector<int> idsOf(const std::vector<Reservation>& v) {
    std::vector<int> ids;
    for (const Reservation& r : v) ids.push_back(r.id);
    std::sort(ids.begin(), ids.end());
    return ids;
}

int main() {
    // empty index
    {
        ReservationIndex idx({});
        CHECK(idx.size() == 0);
        CHECK(idx.startingIn(0, 1440).empty());
        CHECK(idx.activeAt(600).empty());
        CHECK(idx.findById(1) == nullptr);
    }

    // small example worked out by hand
    {
        std::vector<Reservation> r = {
            {1, 2, 600, 60, 0},     // 10:00 - 11:00
            {2, 2, 610, 30, 0},     // 10:10 - 10:40
            {3, 4, 900, 90, 0}};    // 15:00 - 16:30
        ReservationIndex idx(r);
        CHECK(idx.size() == 3);
        CHECK(idx.countStartingIn(600, 611) == 2);
        CHECK(idx.countStartingIn(611, 900) == 0);
        CHECK(idx.countStartingIn(900, 901) == 1);
        CHECK(idx.startingIn(700, 600).empty());          // from >= to
        CHECK(idx.activeAt(620).size() == 2);             // both are in progress
        CHECK(idx.activeAt(660).empty());                 // the end time is not "active"
        CHECK(idx.slotCount(40) == 2);                    // 600/15 = 40 and 610/15 = 40
        CHECK(idx.slotCount(60) == 1);                    // 900/15 = 60
        CHECK(idx.slotCount(-1) == 0 && idx.slotCount(96) == 0);
        CHECK(idx.findById(3) != nullptr && idx.findById(3)->start == 900);
        CHECK(idx.findById(9) == nullptr);
    }

    // real data: every answer must equal a plain scan of the whole list
    {
        Dataset d;
        std::string err;
        if (loadDataset("data/medium", d, err)) {
            ReservationIndex idx(d.reservations);
            CHECK(idx.size() == d.reservations.size());

            std::mt19937 rng(9);
            bool ok = true;
            for (int q = 0; q < 300; ++q) {
                int a = static_cast<int>(rng() % 1500);
                int b = static_cast<int>(rng() % 1500);     // may be <= a on purpose
                std::vector<int> inRange, active;
                for (const Reservation& r : d.reservations) {
                    if (r.start >= a && r.start < b) inRange.push_back(r.id);
                    if (r.start <= a && r.end() > a) active.push_back(r.id);
                }
                std::sort(inRange.begin(), inRange.end());
                std::sort(active.begin(), active.end());
                if (idsOf(idx.startingIn(a, b)) != inRange)     ok = false;
                if (idx.countStartingIn(a, b) != inRange.size()) ok = false;
                if (idsOf(idx.activeAt(a)) != active)           ok = false;
            }
            CHECK(ok);

            // slots: every reservation is in exactly one slot, the right one
            std::size_t total = 0;
            bool slotsOk = true;
            for (int s = 0; s < ReservationIndex::NUM_SLOTS; ++s) {
                total += idx.slotCount(s);
                for (const Reservation& r : idx.slotReservations(s))
                    if (r.start / ReservationIndex::SLOT_MIN != s) slotsOk = false;
            }
            CHECK(total == d.reservations.size());
            CHECK(slotsOk);

            // every reservation can be found by its id
            bool idsOk = true;
            for (const Reservation& r : d.reservations) {
                const Reservation* p = idx.findById(r.id);
                if (!p || p->start != r.start || p->party != r.party) idsOk = false;
            }
            CHECK(idsOk);
        } else {
            CHECK(false);  // run "make data" first
        }
    }

    // adversarial: all 10000 reservations start at 18:30 (1110) and last 90 minutes
    {
        Dataset d;
        std::string err;
        if (loadDataset("data/adversarial/equal", d, err)) {
            ReservationIndex idx(d.reservations);
            CHECK(idx.countStartingIn(1110, 1111) == 10000);
            CHECK(idx.countStartingIn(0, 1110) == 0);
            CHECK(idx.activeAt(1150).size() == 10000);
            CHECK(idx.activeAt(1200).empty());
        } else {
            CHECK(false);  // run "make data" first
        }
    }

    return TEST_SUMMARY();
}