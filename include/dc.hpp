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

// ---------- Fast power ----------
#include "model.hpp"

// Naive: e multiplications, Theta(e). Used as the checker.
inline unsigned long long powerNaive(unsigned long long b, unsigned e) {
    unsigned long long r = 1;
    for (unsigned i = 0; i < e; ++i) r *= b;
    return r;
}

// Divide and conquer: T(e) = T(e/2) + Theta(1) = Theta(log e).
// The result must fit in 64 bits (no overflow protection).
inline unsigned long long powerFast(unsigned long long b, unsigned e) {
    if (e == 0) return 1;
    unsigned long long half = powerFast(b, e / 2);
    return (e % 2 == 0) ? half * half : half * half * b;
}

// Same idea as a loop (look at the bits of e from the lowest one up).
inline unsigned long long powerIter(unsigned long long b, unsigned e) {
    unsigned long long r = 1;
    while (e > 0) {
        if (e & 1u) r *= b;
        b *= b;
        e >>= 1;
    }
    return r;
}

// (b ^ e) mod m, Theta(log e). A 128-bit intermediate avoids overflow.
inline unsigned long long powMod(unsigned long long b, unsigned long long e,
                                 unsigned long long m) {
    if (m == 1) return 0;
    unsigned long long r = 1;
    b %= m;
    while (e > 0) {
        if (e & 1ULL)
            r = static_cast<unsigned long long>(
                    static_cast<unsigned __int128>(r) * b % m);
        b = static_cast<unsigned long long>(
                static_cast<unsigned __int128>(b) * b % m);
        e >>= 1;
    }
    return r;
}

// Checksum code for a reservation, 0..1000000006.
inline unsigned long long reservationCode(const Reservation& r) {
    unsigned long long x = r.id * 131ULL + r.party * 7ULL + r.start;
    return powMod(x, 65537ULL, 1000000007ULL);
}

// ---------- Celebrity problem ----------
// knows[a][b] == true means person a knows person b.
// A celebrity is known by everyone else and knows nobody. Returns -1 if none.
using KnowsMatrix = std::vector<std::vector<bool>>;

struct CelebResult {
    int person;          // index of the celebrity, or -1
    long long calls;     // how many times knows[][] was read
};

namespace detail {

// true if c is known by everyone else and knows nobody (counts every read)
inline bool isCelebrity(const KnowsMatrix& k, int c, long long& calls) {
    int n = static_cast<int>(k.size());
    for (int i = 0; i < n; ++i) {
        if (i == c) continue;
        calls += 2;
        if (k[c][i] || !k[i][c]) return false;
    }
    return true;
}

// Divide and conquer on [lo, hi): returns the only possible candidate.
// T(n) = 2T(n/2) + Theta(1) = Theta(n)
inline int celebCandidate(const KnowsMatrix& k, int lo, int hi, long long& calls) {
    if (hi - lo == 1) return lo;
    int mid = lo + (hi - lo) / 2;
    int a = celebCandidate(k, lo, mid, calls);
    int b = celebCandidate(k, mid, hi, calls);
    ++calls;
    return k[a][b] ? b : a;      // if a knows b, a is out; otherwise b is out
}

} // namespace detail

// Brute force, Theta(n^2): test everyone.
inline CelebResult celebrityBrute(const KnowsMatrix& k) {
    long long calls = 0;
    int n = static_cast<int>(k.size());
    for (int c = 0; c < n; ++c)
        if (detail::isCelebrity(k, c, calls)) return {c, calls};
    return {-1, calls};
}

// Elimination, at most 3(n-1) reads.
inline CelebResult celebrityElim(const KnowsMatrix& k) {
    long long calls = 0;
    int n = static_cast<int>(k.size());
    if (n == 0) return {-1, 0};
    int cand = 0;
    for (int i = 1; i < n; ++i) {
        ++calls;
        if (k[cand][i]) cand = i;    // cand knows i, so cand cannot be the celebrity
    }
    return {detail::isCelebrity(k, cand, calls) ? cand : -1, calls};
}

// Divide and conquer candidate search, then one verification pass.
inline CelebResult celebrityDC(const KnowsMatrix& k) {
    long long calls = 0;
    int n = static_cast<int>(k.size());
    if (n == 0) return {-1, 0};
    int cand = detail::celebCandidate(k, 0, n, calls);
    return {detail::isCelebrity(k, cand, calls) ? cand : -1, calls};
}