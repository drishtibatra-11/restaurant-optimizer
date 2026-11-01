#pragma once
#include <cstddef>
#include <vector>

// =====================================================================
// Backtracking: build a solution one step at a time, and undo the last
// step (backtrack) as soon as it can no longer lead to a valid answer.
// =====================================================================

// ---------- N-Queens ----------
struct QueensResult {
    long long solutions = 0;    // number of ways found
    long long nodes = 0;        // how many partial boards were looked at
    std::vector<int> first;     // first[row] = column of the queen in that row
};

namespace detail {

inline void queensRec(int n, int row, std::vector<int>& col, std::vector<bool>& usedCol,
                      std::vector<bool>& diagA, std::vector<bool>& diagB,
                      bool stopAtFirst, QueensResult& r) {
    ++r.nodes;
    if (row == n) {
        if (r.solutions == 0) r.first = col;
        ++r.solutions;
        return;
    }
    for (int c = 0; c < n; ++c) {
        if (usedCol[c] || diagA[row + c] || diagB[row - c + n - 1]) continue;   // attacked
        col[row] = c;                                                           // place
        usedCol[c] = diagA[row + c] = diagB[row - c + n - 1] = true;
        queensRec(n, row + 1, col, usedCol, diagA, diagB, stopAtFirst, r);
        usedCol[c] = diagA[row + c] = diagB[row - c + n - 1] = false;           // undo
        if (stopAtFirst && r.solutions > 0) return;
    }
}

} // namespace detail

// Counts all solutions (or stops at the first one). n < 1 gives no solutions.
inline QueensResult nQueens(int n, bool stopAtFirst = false) {
    QueensResult r;
    if (n < 1) return r;
    std::vector<int> col(n, -1);
    std::vector<bool> usedCol(n, false), diagA(2 * n - 1, false), diagB(2 * n - 1, false);
    detail::queensRec(n, 0, col, usedCol, diagA, diagB, stopAtFirst, r);
    return r;
}

// ---------- Subset sum ----------
struct SubsetResult {
    bool ok = true;                       // false if an item is negative
    long long count = 0;                  // number of subsets that add up to the target
    long long nodes = 0;                  // how many partial choices were looked at
    std::vector<std::size_t> first;       // indices of the first subset found
};

namespace detail {

inline void subsetRec(const std::vector<int>& a, const std::vector<long long>& suffix,
                      std::size_t i, long long sum, long long target, bool stopAtFirst,
                      std::vector<std::size_t>& chosen, SubsetResult& r) {
    ++r.nodes;
    if (sum > target) return;                       // too big already: items are not negative
    if (sum + suffix[i] < target) return;           // even taking everything left is not enough
    if (i == a.size()) {                            // here sum == target
        if (r.count == 0) r.first = chosen;
        ++r.count;
        return;
    }
    chosen.push_back(i);                            // take item i
    subsetRec(a, suffix, i + 1, sum + a[i], target, stopAtFirst, chosen, r);
    chosen.pop_back();                              // undo
    if (stopAtFirst && r.count > 0) return;
    subsetRec(a, suffix, i + 1, sum, target, stopAtFirst, chosen, r);   // skip item i
}

} // namespace detail

// Counts the subsets of `a` (non-negative numbers) with sum == target.
inline SubsetResult subsetSum(const std::vector<int>& a, long long target, bool stopAtFirst = false) {
    SubsetResult r;
    for (int x : a) if (x < 0) { r.ok = false; return r; }
    std::vector<long long> suffix(a.size() + 1, 0);       // suffix[i] = a[i] + ... + a[n-1]
    for (std::size_t i = a.size(); i-- > 0;) suffix[i] = suffix[i + 1] + a[i];
    std::vector<std::size_t> chosen;
    detail::subsetRec(a, suffix, 0, 0, target, stopAtFirst, chosen, r);
    return r;
}