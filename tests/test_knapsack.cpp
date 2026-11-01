#include "csv_loader.hpp"
#include "dp.hpp"
#include "test.hpp"
#include <cmath>
#include <random>
#include <set>
#include <string>
#include <vector>

// Oracle: try every subset (only for small n)
static long long bruteKnap(const std::vector<KnapItem>& it, int cap) {
    std::size_t n = it.size();
    long long best = 0;
    for (unsigned mask = 0; mask < (1u << n); ++mask) {
        long long w = 0, v = 0;
        for (std::size_t i = 0; i < n; ++i)
            if (mask >> i & 1) { w += it[i].weight; v += it[i].value; }
        if (w <= cap && v > best) best = v;
    }
    return best;
}

// The chosen set must be real: distinct valid indices, fits, and adds up to the result
static bool resultOk(const std::vector<KnapItem>& it, const KnapResult& r, int cap) {
    std::set<std::size_t> seen;
    long long w = 0, v = 0;
    for (std::size_t i : r.chosen) {
        if (i >= it.size() || !seen.insert(i).second) return false;
        w += it[i].weight;
        v += it[i].value;
    }
    return w <= cap && w == r.weightUsed && v == r.value;
}

int main() {
    // no items, zero capacity, negative capacity
    {
        std::vector<KnapItem> none;
        CHECK(knapsackDP(none, 10).value == 0 && knapsackDP(none, 10).chosen.empty());
        CHECK(knapsackValueOnly(none, 10) == 0);
        std::vector<KnapItem> one = {{5, 9}};
        CHECK(knapsackDP(one, 0).value == 0);
        CHECK(knapsackDP(one, -3).value == 0 && knapsackValueOnly(one, -3) == 0);
        CHECK(knapsackGreedy(one, -3).value == 0);
    }

    // an item that is too heavy is never taken; a weight-0 item is always free
    {
        std::vector<KnapItem> it = {{50, 100}, {0, 7}, {3, 4}};
        KnapResult r = knapsackDP(it, 10);
        CHECK(r.value == 11 && resultOk(it, r, 10));
        CHECK(knapsackValueOnly(it, 10) == 11);
        CHECK(knapsackGreedy(it, 10).value == 11);
    }

    // classic example: weights 10, 20, 30 and values 60, 100, 120, capacity 50
    {
        std::vector<KnapItem> it = {{10, 60}, {20, 100}, {30, 120}};
        KnapResult dp = knapsackDP(it, 50), gr = knapsackGreedy(it, 50);
        CHECK(dp.value == 220 && dp.weightUsed == 50);       // items 2 and 3
        CHECK(dp.chosen.size() == 2 && dp.chosen[0] == 1 && dp.chosen[1] == 2);
        CHECK(knapsackValueOnly(it, 50) == 220);
        CHECK(gr.value == 160);                              // greedy is not optimal here
    }

    // 300 random cases against the brute force
    {
        std::mt19937 rng(71);
        bool dpOk = true, rowOk = true, greedyOk = true, greedyNever = true, gap = false;
        for (int trial = 0; trial < 300; ++trial) {
            int n = static_cast<int>(rng() % 13);
            std::vector<KnapItem> it;
            for (int i = 0; i < n; ++i)
                it.push_back({static_cast<int>(rng() % 16), static_cast<long long>(rng() % 31)});
            int cap = static_cast<int>(rng() % 41);
            long long expect = bruteKnap(it, cap);
            KnapResult d = knapsackDP(it, cap), g = knapsackGreedy(it, cap);
            if (d.value != expect || !resultOk(it, d, cap)) dpOk = false;
            if (knapsackValueOnly(it, cap) != expect) rowOk = false;
            if (!resultOk(it, g, cap)) greedyOk = false;
            if (g.value > expect) greedyNever = false;
            if (g.value < expect) gap = true;
        }
        CHECK(dpOk);
        CHECK(rowOk);
        CHECK(greedyOk);
        CHECK(greedyNever);   // greedy can never beat the optimum
        CHECK(gap);           // and it really is worse on some inputs
    }

    // restaurant use: pick specials from the real dishes within 60 prep-minutes
    {
        Dataset d;
        std::string err;
        if (loadDataset("data/medium", d, err)) {
            std::vector<KnapItem> it;
            for (const Dish& x : d.dishes)
                it.push_back({x.prepTime, std::llround(x.profit * 100)});   // profit in cents
            KnapResult dp = knapsackDP(it, 60), gr = knapsackGreedy(it, 60);
            CHECK(dp.value > 0 && resultOk(it, dp, 60));
            CHECK(knapsackValueOnly(it, 60) == dp.value);
            CHECK(resultOk(it, gr, 60) && gr.value <= dp.value);
            std::cout << "  specials (60 min): DP " << dp.value << " vs greedy " << gr.value
                      << " (cents of profit)\n";
        } else {
            CHECK(false);  // run "make data" first
        }
    }

    return TEST_SUMMARY();
}