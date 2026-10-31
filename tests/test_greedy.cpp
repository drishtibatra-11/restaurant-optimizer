#include "csv_loader.hpp"
#include "greedy.hpp"
#include "test.hpp"
#include <algorithm>
#include <random>
#include <string>
#include <vector>

// Oracle for activity selection: try every subset (only for small n)
static std::size_t bruteActivity(const std::vector<Reservation>& r) {
    std::size_t n = r.size(), best = 0;
    for (unsigned mask = 0; mask < (1u << n); ++mask) {
        bool ok = true;
        for (std::size_t i = 0; i < n && ok; ++i)
            if (mask >> i & 1)
                for (std::size_t j = i + 1; j < n; ++j)
                    if ((mask >> j & 1) && overlaps(r[i], r[j])) { ok = false; break; }
        if (ok) best = std::max(best, static_cast<std::size_t>(__builtin_popcount(mask)));
    }
    return best;
}

// Oracle for partitioning: the largest number of reservations overlapping at one moment
static int maxDepth(const std::vector<Reservation>& r) {
    std::vector<std::pair<int, int>> ev;   // (time, +1 start / -1 end); ends sort first
    for (const Reservation& x : r) { ev.push_back({x.start, 1}); ev.push_back({x.end(), -1}); }
    std::sort(ev.begin(), ev.end());
    int cur = 0, best = 0;
    for (auto& e : ev) { cur += e.second; best = std::max(best, cur); }
    return best;
}

static bool chosenOk(const std::vector<Reservation>& r, const std::vector<std::size_t>& c) {
    for (std::size_t a = 0; a < c.size(); ++a) {
        if (c[a] >= r.size()) return false;
        for (std::size_t b = a + 1; b < c.size(); ++b)
            if (overlaps(r[c[a]], r[c[b]])) return false;
    }
    return true;
}

static bool assignOk(const std::vector<Reservation>& r, const PartitionResult& p) {
    if (p.assign.size() != r.size()) return false;
    for (std::size_t i = 0; i < r.size(); ++i) {
        if (p.assign[i] < 0 || p.assign[i] >= p.resources) return false;
        for (std::size_t j = i + 1; j < r.size(); ++j)
            if (p.assign[i] == p.assign[j] && overlaps(r[i], r[j])) return false;
    }
    return true;
}

int main() {
    // empty input
    CHECK(activitySelection({}).empty());
    CHECK(intervalPartition({}).resources == 0);

    // hand example: 10:00-11:00, 10:30-11:30, 11:00-12:00, 10:00-13:00
    {
        std::vector<Reservation> r = {
            {1, 2, 600, 60, 0}, {2, 2, 630, 60, 0}, {3, 2, 660, 60, 0}, {4, 2, 600, 180, 0}};
        std::vector<std::size_t> c = activitySelection(r);
        CHECK(c.size() == 2 && c[0] == 0 && c[1] == 2);   // reservations 1 and 3
        CHECK(intervalPartition(r).resources == 3);       // 3 overlap at 10:30
    }

    // touching intervals do not overlap: one table is enough
    {
        std::vector<Reservation> r = {{1, 2, 600, 60, 0}, {2, 2, 660, 60, 0}};
        CHECK(!overlaps(r[0], r[1]));
        CHECK(activitySelection(r).size() == 2);
        CHECK(intervalPartition(r).resources == 1);
    }

    // 300 random cases against the oracles
    {
        std::mt19937 rng(31);
        bool actOk = true, partOk = true;
        for (int trial = 0; trial < 300; ++trial) {
            int n = 1 + static_cast<int>(rng() % 12);
            std::vector<Reservation> r;
            for (int i = 0; i < n; ++i)
                r.push_back({i + 1, 2, static_cast<int>(rng() % 200),
                             1 + static_cast<int>(rng() % 60), 0});

            std::vector<std::size_t> c = activitySelection(r);
            if (c.size() != bruteActivity(r) || !chosenOk(r, c)) actOk = false;

            PartitionResult p = intervalPartition(r);
            if (p.resources != maxDepth(r) || !assignOk(r, p)) partOk = false;
        }
        CHECK(actOk);
        CHECK(partOk);
    }

    // real data
    {
        Dataset d;
        std::string err;
        if (loadDataset("data/medium", d, err)) {
            std::vector<std::size_t> c = activitySelection(d.reservations);
            CHECK(!c.empty() && chosenOk(d.reservations, c));
            PartitionResult p = intervalPartition(d.reservations);
            CHECK(p.resources == maxDepth(d.reservations));
            CHECK(assignOk(d.reservations, p));
        } else {
            CHECK(false);  // run "make data" first
        }
    }

    // adversarial: 10000 identical reservations
    {
        Dataset d;
        std::string err;
        if (loadDataset("data/adversarial/equal", d, err)) {
            CHECK(activitySelection(d.reservations).size() == 1);
            CHECK(intervalPartition(d.reservations).resources == 10000);
        } else {
            CHECK(false);  // run "make data" first
        }
    }

    return TEST_SUMMARY();
}