#include "bnb.hpp"
#include "csv_loader.hpp"
#include "test.hpp"
#include <iomanip>
#include <iostream>
#include <random>
#include <set>
#include <string>
#include <vector>

// Oracle: try every way to give each row its own column (no pruning at all)
static void bruteRec(const CostMatrix& c, std::size_t row, std::vector<bool>& used,
                     long long cost, long long& best) {
    if (row == c.size()) { if (cost < best) best = cost; return; }
    for (std::size_t j = 0; j < c[row].size(); ++j) {
        if (used[j] || c[row][j] >= INF) continue;
        used[j] = true;
        bruteRec(c, row + 1, used, cost + c[row][j], best);
        used[j] = false;
    }
}

static long long bruteForce(const CostMatrix& c) {
    std::vector<bool> used(c.empty() ? 0 : c[0].size(), false);
    long long best = INF;
    bruteRec(c, 0, used, 0, best);
    return best;
}

// the assignment must use distinct, allowed tables and add up to the reported cost
static bool assignOk(const CostMatrix& c, const AssignResult& r) {
    if (!r.feasible) return r.assign.empty();
    if (r.assign.size() != c.size()) return false;
    std::set<int> seen;
    long long sum = 0;
    for (std::size_t i = 0; i < c.size(); ++i) {
        int j = r.assign[i];
        if (j < 0 || j >= static_cast<int>(c[i].size()) || !seen.insert(j).second) return false;
        if (c[i][j] >= INF) return false;
        sum += c[i][j];
    }
    return sum == r.cost;
}

static CostMatrix randomCosts(std::mt19937& rng, int n, int m, int pctForbidden) {
    CostMatrix c(n, std::vector<long long>(m, 0));
    for (auto& row : c)
        for (long long& x : row)
            x = (static_cast<int>(rng() % 100) < pctForbidden) ? INF : 1 + static_cast<long long>(rng() % 30);
    return c;
}

int main() {
    const Strategy all[] = {Strategy::FIFO, Strategy::LIFO, Strategy::LeastCost};

    // invalid and trivial inputs
    {
        CostMatrix ragged = {{1, 2}, {3}};
        CHECK(!branchAndBound(ragged, Strategy::FIFO).ok && !assignDP(ragged).ok && !assignGreedy(ragged).ok);
        CostMatrix wide(1, std::vector<long long>(31, 1));          // more than 30 tables
        CHECK(!branchAndBound(wide, Strategy::LIFO).ok);
        CostMatrix none;                                             // nobody to seat
        for (Strategy s : all) CHECK(branchAndBound(none, s).feasible && branchAndBound(none, s).cost == 0);
        CHECK(assignDP(none).feasible && assignDP(none).cost == 0);
        CostMatrix tooMany = {{1}, {2}};                             // 2 parties, 1 table
        for (Strategy s : all) CHECK(branchAndBound(tooMany, s).ok && !branchAndBound(tooMany, s).feasible);
        CHECK(!assignDP(tooMany).feasible && !assignGreedy(tooMany).feasible);
    }

    // textbook example: best cost is 2 + 6 + 1 + 4 = 13
    {
        CostMatrix c = {{9, 2, 7, 8}, {6, 4, 3, 7}, {5, 8, 1, 8}, {7, 6, 9, 4}};
        CHECK(bruteForce(c) == 13);
        AssignResult d = assignDP(c);
        CHECK(d.cost == 13 && assignOk(c, d));
        for (Strategy s : all) {
            AssignResult r = branchAndBound(c, s);
            CHECK(r.feasible && r.cost == 13 && assignOk(c, r));
            CHECK(r.assign == std::vector<int>({1, 0, 2, 3}));
            CHECK(branchAndBound(c, s, true).cost == 13);
        }
    }

    // nobody can be seated: two parties only fit at the same single table
    {
        CostMatrix c = {{5, INF, INF}, {7, INF, INF}};
        CHECK(!assignDP(c).feasible && !assignGreedy(c).feasible);
        for (Strategy s : all) {
            AssignResult r = branchAndBound(c, s);
            CHECK(r.ok && !r.feasible && r.cost >= INF && r.assign.empty());
        }
    }

    // all costs equal: every strategy finds a valid assignment of cost 5 * n
    {
        CostMatrix c(5, std::vector<long long>(6, 5));
        for (Strategy s : all) {
            AssignResult r = branchAndBound(c, s);
            CHECK(r.cost == 25 && assignOk(c, r));
        }
    }

    // 300 random small cases against the brute force, the DP, and the greedy
    {
        std::mt19937 rng(131);
        bool costOk = true, feasOk = true, validOk = true, dpOk = true;
        bool greedyOk = true, gap = false;
        for (int trial = 0; trial < 300; ++trial) {
            int n = 1 + static_cast<int>(rng() % 6);
            int m = n + static_cast<int>(rng() % 3);
            CostMatrix c = randomCosts(rng, n, m, 15);
            long long expect = bruteForce(c);
            bool feasible = expect < INF;
            AssignResult d = assignDP(c);
            if (d.feasible != feasible || (feasible && d.cost != expect) || !assignOk(c, d)) dpOk = false;
            for (Strategy s : all)
                for (bool warm : {false, true}) {
                    AssignResult r = branchAndBound(c, s, warm);
                    if (r.feasible != feasible) feasOk = false;
                    if (feasible && r.cost != expect) costOk = false;
                    if (!assignOk(c, r)) validOk = false;
                }
            AssignResult g = assignGreedy(c);
            if (g.feasible) {
                if (!feasible || g.cost < expect || !assignOk(c, g)) greedyOk = false;
                if (g.cost > expect) gap = true;
            }
        }
        CHECK(costOk);
        CHECK(feasOk);
        CHECK(validOk);
        CHECK(dpOk);
        CHECK(greedyOk);   // greedy never beats the optimum
        CHECK(gap);        // and it is really worse on some inputs
    }

    // bigger cases (6 reservations, 9 tables): all three strategies against the DP,
    // and a count of how much work each one does
    {
        std::mt19937 rng(132);
        bool costOk = true, validOk = true;
        long long expanded[3] = {0, 0, 0}, generated[3] = {0, 0, 0}, peak[3] = {0, 0, 0};
        for (int trial = 0; trial < 100; ++trial) {
            CostMatrix c = randomCosts(rng, 6, 9, 10);
            AssignResult d = assignDP(c);
            for (int k = 0; k < 3; ++k) {
                AssignResult r = branchAndBound(c, all[k]);
                if (r.feasible != d.feasible || (d.feasible && r.cost != d.cost)) costOk = false;
                if (!assignOk(c, r)) validOk = false;
                expanded[k] += r.expanded;
                generated[k] += r.generated;
                peak[k] = std::max(peak[k], r.peakFrontier);
            }
        }
        CHECK(costOk);
        CHECK(validOk);
        CHECK(expanded[2] <= expanded[0]);                   // least-cost expands the fewest nodes
        CHECK(expanded[2] <= expanded[1]);
        CHECK(expanded[0] < 100 * fullTreeNodes(6, 9));     // pruning does something
        std::cout << "  100 random cases, 6 reservations x 9 tables (full tree = "
                  << fullTreeNodes(6, 9) << " nodes each):\n";
        for (int k = 0; k < 3; ++k)
            std::cout << "    " << std::left << std::setw(10) << strategyName(all[k]) << std::right
                      << " expanded " << std::setw(8) << expanded[k] << "  generated " << std::setw(8)
                      << generated[k] << "  peak frontier " << std::setw(7) << peak[k] << "\n";
    }

    // restaurant use: the 18:00 to 20:00 rush, up to 6 parties on the first 10 tables
    {
        Dataset d;
        std::string err;
        if (loadDataset("data/medium", d, err)) {
            std::vector<Reservation> rush;
            for (const Reservation& r : d.reservations)
                if (r.start >= 1080 && r.start < 1200 && rush.size() < 6) rush.push_back(r);
            std::vector<Table> tables(d.tables.begin(), d.tables.begin() + 10);
            CostMatrix c = seatingCosts(rush, tables, kitchenWalk(tables));
            CHECK(rush.size() >= 3);
            AssignResult dp = assignDP(c), gr = assignGreedy(c);
            bool same = true, valid = true;
            std::cout << "  rush hour: " << rush.size() << " parties, " << tables.size() << " tables, "
                      << (dp.feasible ? "optimal cost " + std::to_string(dp.cost) : "no valid seating")
                      << ", greedy " << (gr.feasible ? std::to_string(gr.cost) : "failed") << "\n";
            for (Strategy s : all) {
                AssignResult r = branchAndBound(c, s);
                if (r.feasible != dp.feasible || (dp.feasible && r.cost != dp.cost)) same = false;
                if (!assignOk(c, r)) valid = false;
                std::cout << "    " << std::left << std::setw(10) << strategyName(s) << std::right
                          << " expanded " << std::setw(6) << r.expanded << "  generated " << std::setw(6)
                          << r.generated << "  peak " << std::setw(5) << r.peakFrontier << "  "
                          << std::fixed << std::setprecision(3) << r.ms << " ms\n";
            }
            CHECK(same);
            CHECK(valid);
            CHECK(!gr.feasible || !dp.feasible || gr.cost >= dp.cost);
        } else {
            CHECK(false);  // run "make data" first
        }
    }

    // adversarial: 10000 identical reservations, only the first 6 are seated
    {
        Dataset d;
        std::string err;
        if (loadDataset("data/adversarial/equal", d, err)) {
            std::vector<Reservation> rush(d.reservations.begin(), d.reservations.begin() + 6);
            std::vector<Table> tables(d.tables.begin(), d.tables.begin() + 10);
            CostMatrix c = seatingCosts(rush, tables, kitchenWalk(tables));
            AssignResult dp = assignDP(c);
            bool same = true;
            for (Strategy s : all) {
                AssignResult r = branchAndBound(c, s);
                if (r.feasible != dp.feasible || (dp.feasible && r.cost != dp.cost)) same = false;
            }
            CHECK(same);
        } else {
            CHECK(false);  // run "make data" first
        }
    }

    return TEST_SUMMARY();
}