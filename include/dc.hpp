#pragma once
#include <cstddef>
#include <vector>

// ---------- Maximum subarray ----------
struct SubResult {
    long long sum;
    long lo;   // first index of the best stretch
    long hi;   // last index of the best stretch (inclusive)
};

namespace detail {

inline SubResult maxSubRec(const std::vector<int>& a, long lo, long hi) {
    // inclusive range [lo, hi]
    if (lo == hi) return {a[lo], lo, lo};
    long mid = lo + (hi - lo) / 2;
    SubResult left  = maxSubRec(a, lo, mid);
    SubResult right = maxSubRec(a, mid + 1, hi);

    // best stretch that crosses the middle: extend left from mid, right from mid+1
    long long sum = 0, bestL = a[mid];
    long li = mid;
    for (long i = mid; i >= lo; --i) {
        sum += a[i];
        if (sum > bestL || i == mid) { bestL = sum; li = i; }
    }
    sum = 0;
    long long bestR = a[mid + 1];
    long ri = mid + 1;
    for (long j = mid + 1; j <= hi; ++j) {
        sum += a[j];
        if (sum > bestR || j == mid + 1) { bestR = sum; ri = j; }
    }
    SubResult cross = {bestL + bestR, li, ri};

    if (left.sum >= right.sum && left.sum >= cross.sum) return left;
    if (right.sum >= cross.sum) return right;
    return cross;
}

} // namespace detail

// Divide and conquer, Theta(n log n). Requires a non-empty vector.
inline SubResult maxSubarrayDC(const std::vector<int>& a) {
    return detail::maxSubRec(a, 0, static_cast<long>(a.size()) - 1);
}

// Kadane, O(n). Requires a non-empty vector.
inline SubResult maxSubarrayKadane(const std::vector<int>& a) {
    long long best = a[0], cur = a[0];
    long bestLo = 0, bestHi = 0, curLo = 0;
    for (long i = 1; i < static_cast<long>(a.size()); ++i) {
        if (cur < 0) { cur = a[i]; curLo = i; }
        else         { cur += a[i]; }
        if (cur > best) { best = cur; bestLo = curLo; bestHi = i; }
    }
    return {best, bestLo, bestHi};
}

// Brute force O(n^2), used only by tests as the oracle.
inline long long maxSubarrayBrute(const std::vector<int>& a) {
    long long best = a[0];
    for (std::size_t i = 0; i < a.size(); ++i) {
        long long s = 0;
        for (std::size_t j = i; j < a.size(); ++j) {
            s += a[j];
            if (s > best) best = s;
        }
    }
    return best;
}