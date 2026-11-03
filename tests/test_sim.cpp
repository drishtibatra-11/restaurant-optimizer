#include "csv_loader.hpp"
#include "sim.hpp"
#include "test.hpp"
#include <algorithm>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

// A seating result is valid when: seated parties fit their table, wait no longer than the
// patience, never start before their booking, and no two parties overlap at one table.
static bool seatingValid(const std::vector<Reservation>& res, const std::vector<Table>& tables,
                         const SeatStats& st, int patience) {
    if (st.tableOf.size() != res.size() || st.seated + st.lost != st.total) return false;
    std::map<int, std::vector<std::pair<int, int>>> perTable;
    std::size_t seated = 0;
    for (std::size_t i = 0; i < res.size(); ++i) {
        int t = st.tableOf[i];
        if (t < 0) { if (st.seatAt[i] != -1) return false; continue; }
        ++seated;
        if (t >= static_cast<int>(tables.size()) || tables[t].seats < res[i].party) return false;
        int wait = st.seatAt[i] - res[i].start;
        if (wait < 0 || wait > patience) return false;
        perTable[t].push_back({st.seatAt[i], st.seatAt[i] + res[i].dur});
    }
    if (seated != st.seated) return false;
    for (auto& kv : perTable) {
        std::sort(kv.second.begin(), kv.second.end());
        for (std::size_t i = 1; i < kv.second.size(); ++i)
            if (kv.second[i].first < kv.second[i - 1].second) return false;
    }
    return st.maxWait <= patience;
}

// Kitchen: every dish starts after it arrived, takes exactly its prep time,
// and no station ever cooks more dishes at once than it has cooks.
static bool kitchenValid(const Dataset& d, const KitchenStats& ks) {
    std::size_t expectItems = 0;
    for (const Order& o : d.orders) expectItems += o.dishes.size();
    if (ks.items != expectItems || ks.slots.size() != expectItems) return false;
    std::vector<std::vector<std::pair<int, int>>> events(d.stations.size());   // (time, +1/-1)
    for (const KitchenSlot& s : ks.slots) {
        if (s.start < s.arrival || s.finish <= s.start) return false;
        events[s.station].push_back({s.start, 1});
        events[s.station].push_back({s.finish, -1});
    }
    for (std::size_t s = 0; s < events.size(); ++s) {
        std::sort(events[s].begin(), events[s].end());          // at equal times, -1 comes first
        int cur = 0;
        for (auto& e : events[s]) {
            cur += e.second;
            if (cur > d.stations[s].capacity) return false;
        }
    }
    return true;
}

static Dataset smallKitchen() {
    Dataset d;
    d.stations = {{1, 1}};
    d.dishes = {{1, 10, 1, 1, 1}, {2, 5, 1, 1, 1}, {3, 1, 1, 1, 1}};
    d.orders = {{1, 1, {1}, 0}, {2, 2, {2}, 1}, {3, 3, {3}, 2}};
    return d;
}

int main() {
    const SeatPolicy policies[] = {SeatPolicy::FirstFit, SeatPolicy::EarliestFit, SeatPolicy::TightFit};

    // ---------- seating: hand example ----------
    {
        std::vector<Table> tables = {{1, 4, 1}, {2, 2, 1}};
        std::vector<Reservation> res = {{1, 2, 100, 60, 0}, {2, 4, 120, 60, 0}};
        SeatStats ff = simulateSeating(res, tables, SeatPolicy::FirstFit, 30);
        // FirstFit puts the party of 2 at the 4-seat table, so the party of 4 waits 40 minutes: it leaves
        CHECK(ff.seated == 1 && ff.lost == 1 && ff.tableOf[0] == 0 && ff.tableOf[1] == -1);
        for (SeatPolicy p : {SeatPolicy::EarliestFit, SeatPolicy::TightFit}) {
            SeatStats s = simulateSeating(res, tables, p, 30);
            CHECK(s.seated == 2 && s.lost == 0 && s.tableOf[0] == 1 && s.tableOf[1] == 0);
            CHECK(s.avgWait == 0 && s.maxWait == 0 && s.wastedSeatMinutes == 0);
        }
        SeatStats patient = simulateSeating(res, tables, SeatPolicy::FirstFit, 60);
        CHECK(patient.seated == 2 && patient.maxWait == 40 && patient.avgWait == 20.0);
    }

    // ---------- seating: edge cases ----------
    {
        CHECK(simulateSeating({}, {{1, 4, 1}}, SeatPolicy::FirstFit).total == 0);
        std::vector<Reservation> res = {{1, 5, 100, 60, 0}};
        CHECK(simulateSeating(res, {{1, 4, 1}}, SeatPolicy::TightFit).lost == 1);       // too big
        CHECK(simulateSeating(res, {}, SeatPolicy::EarliestFit).lost == 1);             // no tables
    }

    // ---------- seating: 200 random cases, every policy must give a valid result ----------
    {
        std::mt19937 rng(171);
        bool valid = true, patienceOk = true;
        for (int trial = 0; trial < 200; ++trial) {
            std::vector<Table> tables;
            int nt = 1 + static_cast<int>(rng() % 6);
            for (int i = 0; i < nt; ++i) tables.push_back({i + 1, 2 * (1 + static_cast<int>(rng() % 4)), 1});
            std::vector<Reservation> res;
            int nr = static_cast<int>(rng() % 25);
            for (int i = 0; i < nr; ++i)
                res.push_back({i + 1, 1 + static_cast<int>(rng() % 8), static_cast<int>(rng() % 200),
                               30 + static_cast<int>(rng() % 90), 0});
            int patience = static_cast<int>(rng() % 60);
            for (SeatPolicy p : policies)
                if (!seatingValid(res, tables, simulateSeating(res, tables, p, patience), patience)) valid = false;
            // infinite patience: nobody is lost unless no table is big enough
            int maxSeats = 0;
            for (const Table& t : tables) maxSeats = std::max(maxSeats, t.seats);
            std::size_t tooBig = 0;
            for (const Reservation& r : res) if (r.party > maxSeats) ++tooBig;
            for (SeatPolicy p : policies)
                if (simulateSeating(res, tables, p, 1000000).lost != tooBig) patienceOk = false;
        }
        CHECK(valid);
        CHECK(patienceOk);
    }

    // ---------- kitchen: hand example (one cook; dishes of 10, 5 and 1 minutes arrive at 0, 1, 2) ----------
    {
        Dataset d = smallKitchen();
        KitchenStats f = simulateKitchen(d, KitchenPolicy::FIFO), s = simulateKitchen(d, KitchenPolicy::ShortestFirst);
        CHECK(kitchenValid(d, f) && kitchenValid(d, s));
        CHECK(f.makespan == 16 && s.makespan == 16);
        CHECK(f.maxTicket == 14 && s.maxTicket == 15);                       // FIFO tickets 10, 14, 14
        CHECK(f.avgTicket > 12.66 && f.avgTicket < 12.67);                   // 38 / 3
        CHECK(s.avgTicket > 11.33 && s.avgTicket < 11.34);                   // 34 / 3
        CHECK(f.avgItemWait > 7.33 && f.avgItemWait < 7.34);                 // (0+9+13)/3
        CHECK(s.avgItemWait == 6.0);                                         // (0+10+8)/3
        CHECK(f.avgUtilization == 1.0);                                      // the cook never rests
    }

    // two cooks: two dishes that arrive together start together
    {
        Dataset d;
        d.stations = {{1, 2}};
        d.dishes = {{1, 10, 1, 1, 1}};
        d.orders = {{1, 1, {1, 1}, 0}};
        for (KitchenPolicy p : {KitchenPolicy::FIFO, KitchenPolicy::ShortestFirst}) {
            KitchenStats k = simulateKitchen(d, p);
            CHECK(k.makespan == 10 && k.maxItemWait == 0 && k.maxTicket == 10);
        }
    }

    // nothing to cook
    {
        Dataset d;
        d.stations = {{1, 1}};
        KitchenStats k = simulateKitchen(d, KitchenPolicy::FIFO);
        CHECK(k.items == 0 && k.makespan == 0 && k.avgTicket == 0);
    }

    // ---------- kitchen: random datasets ----------
    {
        std::mt19937 rng(172);
        bool valid = true, sameWork = true;
        for (int trial = 0; trial < 100; ++trial) {
            Dataset d;
            int ns = 1 + static_cast<int>(rng() % 3);
            for (int i = 1; i <= ns; ++i) d.stations.push_back({i, 1 + static_cast<int>(rng() % 3)});
            int nd = 1 + static_cast<int>(rng() % 6);
            for (int i = 1; i <= nd; ++i)
                d.dishes.push_back({i, 1 + static_cast<int>(rng() % 20), 1 + static_cast<int>(rng() % ns), 1, 1});
            int no = static_cast<int>(rng() % 30);
            for (int i = 1; i <= no; ++i) {
                std::vector<int> ds;
                for (int k = 0, c = 1 + static_cast<int>(rng() % 3); k < c; ++k) ds.push_back(1 + static_cast<int>(rng() % nd));
                d.orders.push_back({i, i, ds, static_cast<int>(rng() % 60)});
            }
            KitchenStats f = simulateKitchen(d, KitchenPolicy::FIFO), s = simulateKitchen(d, KitchenPolicy::ShortestFirst);
            if (!kitchenValid(d, f) || !kitchenValid(d, s)) valid = false;
            if (f.items != s.items) sameWork = false;
        }
        CHECK(valid);
        CHECK(sameWork);
    }

    // ---------- shocks ----------
    {
        Dataset d;
        d.tables = {{1, 2, 1}, {2, 8, 1}, {3, 4, 2}, {4, 8, 2}};
        d.stations = {{1, 3}, {2, 1}};
        Dataset t = removeLargestTables(d, 2);
        CHECK(t.tables.size() == 2 && t.tables[0].seats == 2 && t.tables[1].seats == 4);   // both 8-seaters gone
        CHECK(removeLargestTables(d, 99).tables.empty() && removeLargestTables(d, 0).tables.size() == 4);
        Dataset s = reduceStation(d, 1, 1);
        CHECK(s.stations[0].capacity == 2 && s.stations[1].capacity == 1);
        CHECK(reduceStation(d, 2, 5).stations[1].capacity == 1);                           // never below one cook
        CHECK(d.tables.size() == 4 && d.stations[0].capacity == 3);                        // the original is unchanged
    }

    // ---------- max flow: how many guests fit ----------
    {
        std::vector<Table> tables = {{1, 4, 1}, {2, 4, 1}, {3, 6, 2}};                      // zone 1: 8 seats, zone 2: 6 seats
        CHECK(maxSeatableGuests({}, tables) == 0);
        std::vector<Reservation> anywhere = {{1, 5, 0, 60, 0}, {2, 5, 0, 60, 0}, {3, 5, 0, 60, 0}};
        CHECK(maxSeatableGuests(anywhere, tables) == 14);                                   // 15 guests, only 14 seats
        std::vector<Reservation> zoned = {{1, 6, 0, 60, 1}, {2, 6, 0, 60, 1}, {3, 3, 0, 60, 2}};
        CHECK(maxSeatableGuests(zoned, tables) == 8 + 3);                                   // zone 1 holds 8 of 12
        std::vector<Reservation> ghost = {{1, 4, 0, 60, 9}};                                // zone 9 does not exist
        CHECK(maxSeatableGuests(ghost, tables) == 0);
        std::vector<Reservation> few = {{1, 2, 0, 60, 0}};
        CHECK(maxSeatableGuests(few, tables) == 2);
    }

    // ---------- real data ----------
    {
        Dataset d;
        std::string err;
        if (loadDataset("data/medium", d, err)) {
            bool valid = true;
            for (SeatPolicy p : policies) {
                SeatStats st = simulateSeating(d.reservations, d.tables, p, 30);
                if (!seatingValid(d.reservations, d.tables, st, 30)) valid = false;
                std::cout << "  " << seatPolicyName(p) << ": seated " << st.seated << ", lost " << st.lost
                          << ", avg wait " << st.avgWait << " min\n";
            }
            CHECK(valid);
            KitchenStats f = simulateKitchen(d, KitchenPolicy::FIFO), s = simulateKitchen(d, KitchenPolicy::ShortestFirst);
            CHECK(kitchenValid(d, f) && kitchenValid(d, s));
            CHECK(s.avgItemWait <= f.avgItemWait + 1e-9);                       // shortest-first never waits longer on average
            std::cout << "  kitchen avg ticket: FIFO " << f.avgTicket << " vs ShortestFirst " << s.avgTicket << " min\n";

            // shock: fewer tables can only hurt
            Dataset broken = removeLargestTables(d, d.tables.size() / 3);
            SeatStats before = simulateSeating(d.reservations, d.tables, SeatPolicy::EarliestFit, 30);
            SeatStats after = simulateSeating(d.reservations, broken.tables, SeatPolicy::EarliestFit, 30);
            CHECK(after.seated <= before.seated);
            Dataset slow = reduceStation(d, busiestStation(d), 1);
            CHECK(simulateKitchen(slow, KitchenPolicy::FIFO).avgTicket >= f.avgTicket - 1e-9);
        } else {
            CHECK(false);  // run "make data" first
        }
    }

    // ---------- adversarial: 10000 identical bookings can never use more tables than exist ----------
    {
        Dataset d;
        std::string err;
        if (loadDataset("data/adversarial/equal", d, err)) {
            for (SeatPolicy p : policies) {
                SeatStats st = simulateSeating(d.reservations, d.tables, p, 30);
                CHECK(st.seated <= d.tables.size() && seatingValid(d.reservations, d.tables, st, 30));
            }
        } else {
            CHECK(false);  // run "make data" first
        }
    }

    return TEST_SUMMARY();
}