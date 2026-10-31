#pragma once
#include "model.hpp"
#include <cstddef>
#include <vector>

// All functions need the vector to be sorted in ascending order.
// They return the index of the match, or -1 if there is none.

// Iterative binary search
inline long binarySearchIter(const std::vector<int>& a, int target) {
    long lo = 0, hi = static_cast<long>(a.size()) - 1;
    while (lo <= hi) {
        long mid = lo + (hi - lo) / 2;
        if (a[mid] == target) return mid;
        if (a[mid] < target) lo = mid + 1;
        else                 hi = mid - 1;
    }
    return -1;
}

// Recursive binary search on the inclusive range [lo, hi]
inline long binarySearchRec(const std::vector<int>& a, int target, long lo, long hi) {
    if (lo > hi) return -1;
    long mid = lo + (hi - lo) / 2;
    if (a[mid] == target) return mid;
    if (a[mid] < target) return binarySearchRec(a, target, mid + 1, hi);
    return binarySearchRec(a, target, lo, mid - 1);
}

inline long binarySearchRec(const std::vector<int>& a, int target) {
    return binarySearchRec(a, target, 0, static_cast<long>(a.size()) - 1);
}

// First index whose value is >= target (a.size() if none).
inline std::size_t lowerBound(const std::vector<int>& a, int target) {
    std::size_t lo = 0, hi = a.size();
    while (lo < hi) {
        std::size_t mid = lo + (hi - lo) / 2;
        if (a[mid] < target) lo = mid + 1;
        else                 hi = mid;
    }
    return lo;
}

// Tables must be sorted by seats (ascending).
// Returns the index of the first table with seats >= n, or -1 if none fits.
inline long firstTableAtLeast(const std::vector<Table>& tables, int n) {
    std::size_t lo = 0, hi = tables.size();
    while (lo < hi) {
        std::size_t mid = lo + (hi - lo) / 2;
        if (tables[mid].seats < n) lo = mid + 1;
        else                       hi = mid;
    }
    return lo == tables.size() ? -1 : static_cast<long>(lo);
}