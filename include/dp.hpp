#pragma once
#include "model.hpp"
#include "sort.hpp"
#include <algorithm>
#include <cstddef>
#include <vector>

// ---------- 0/1 knapsack ----------
struct KnapItem {
    int weight;          // must be >= 0 (items with a negative weight are ignored)
    long long value;
};

struct KnapResult {
    long long value = 0;                 // total value of the chosen items
    int weightUsed = 0;                  // total weight of the chosen items
    std::vector<std::size_t> chosen;     // indices into the item list
};

// Full table, Theta(n * W) time and memory. Can rebuild the chosen set.
inline KnapResult knapsackDP(const std::vector<KnapItem>& it, int cap) {
    KnapResult res;
    if (cap < 0) return res;
    std::size_t n = it.size(), W = static_cast<std::size_t>(cap);
    std::vector<std::vector<long long>> dp(n + 1, std::vector<long long>(W + 1, 0));
    for (std::size_t i = 1; i <= n; ++i) {
        const KnapItem& x = it[i - 1];
        for (std::size_t w = 0; w <= W; ++w) {
            dp[i][w] = dp[i - 1][w];                                  // skip the item
            if (x.weight >= 0 && static_cast<std::size_t>(x.weight) <= w) {
                long long take = dp[i - 1][w - x.weight] + x.value;   // take the item
                if (take > dp[i][w]) dp[i][w] = take;
            }
        }
    }
    res.value = dp[n][W];
    // walk back: if the value changed at row i, item i-1 was taken
    std::size_t w = W;
    for (std::size_t i = n; i >= 1; --i)
        if (dp[i][w] != dp[i - 1][w]) {
            res.chosen.push_back(i - 1);
            res.weightUsed += it[i - 1].weight;
            w -= static_cast<std::size_t>(it[i - 1].weight);
        }
    std::reverse(res.chosen.begin(), res.chosen.end());
    return res;
}

// One row only: Theta(n * W) time, O(W) memory. Returns the best value.
inline long long knapsackValueOnly(const std::vector<KnapItem>& it, int cap) {
    if (cap < 0) return 0;
    std::vector<long long> dp(static_cast<std::size_t>(cap) + 1, 0);
    for (const KnapItem& x : it) {
        if (x.weight < 0) continue;
        for (int w = cap; w >= x.weight; --w)       // downwards: each item used once
            dp[w] = std::max(dp[w], dp[w - x.weight] + x.value);
    }
    return dp[cap];
}

// Greedy by value per weight. Theta(n log n). Fast but NOT always optimal.
inline KnapResult knapsackGreedy(const std::vector<KnapItem>& it, int cap) {
    KnapResult res;
    if (cap < 0) return res;
    std::vector<std::size_t> idx;
    for (std::size_t i = 0; i < it.size(); ++i)
        if (it[i].weight >= 0) idx.push_back(i);
    mergeSort(idx, [&](std::size_t a, std::size_t b) {
        const KnapItem& x = it[a];
        const KnapItem& y = it[b];
        if (x.weight == 0 || y.weight == 0) {          // weight 0 counts as infinite ratio
            if (x.weight == y.weight) return x.value > y.value;
            return x.weight == 0;
        }
        return x.value * y.weight > y.value * x.weight;   // compare ratios without dividing
    });
    for (std::size_t i : idx) {
        if (it[i].value <= 0) continue;
        if (res.weightUsed + it[i].weight <= cap) {
            res.chosen.push_back(i);
            res.weightUsed += it[i].weight;
            res.value += it[i].value;
        }
    }
    mergeSort(res.chosen, [](std::size_t a, std::size_t b) { return a < b; });
    return res;
}
#include "dp_more.hpp"