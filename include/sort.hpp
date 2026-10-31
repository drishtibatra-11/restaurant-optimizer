#pragma once
#include <cstddef>
#include <random>
#include <utility>
#include <vector>

// ---------- Merge sort: stable, O(n log n) in every case ----------
namespace detail {

template <class T, class Cmp>
void mergeRec(std::vector<T>& a, std::vector<T>& tmp,
              std::size_t lo, std::size_t hi, Cmp cmp) {
    // sorts the range [lo, hi)
    if (hi - lo < 2) return;
    std::size_t mid = lo + (hi - lo) / 2;
    mergeRec(a, tmp, lo, mid, cmp);
    mergeRec(a, tmp, mid, hi, cmp);

    std::size_t i = lo, j = mid, k = lo;
    while (i < mid && j < hi) {
        // take from the right only if it is strictly smaller (keeps stability)
        if (cmp(a[j], a[i])) tmp[k++] = a[j++];
        else                 tmp[k++] = a[i++];
    }
    while (i < mid) tmp[k++] = a[i++];
    while (j < hi)  tmp[k++] = a[j++];
    for (k = lo; k < hi; ++k) a[k] = tmp[k];
}

} // namespace detail

template <class T, class Cmp>
void mergeSort(std::vector<T>& a, Cmp cmp) {
    std::vector<T> tmp(a);
    detail::mergeRec(a, tmp, 0, a.size(), cmp);
}

// ---------- Quick sort with a choice of pivot rule ----------
enum class Pivot { First, Random, Median3 };

namespace detail {

template <class T, class Cmp>
long choosePivot(const std::vector<T>& a, long lo, long hi, Pivot rule,
                 Cmp cmp, std::mt19937& rng) {
    if (rule == Pivot::First) return lo;
    if (rule == Pivot::Random)
        return std::uniform_int_distribution<long>(lo, hi)(rng);
    // Median3: median of the first, middle and last items
    long x = lo, y = lo + (hi - lo) / 2, z = hi;
    if (cmp(a[y], a[x])) std::swap(x, y);
    if (cmp(a[z], a[y])) {
        std::swap(y, z);
        if (cmp(a[y], a[x])) std::swap(x, y);
    }
    return y;
}

// Lomuto partition on the inclusive range [lo, hi]. Returns the pivot's final index.
template <class T, class Cmp>
long partitionLomuto(std::vector<T>& a, long lo, long hi, Pivot rule,
                     Cmp cmp, std::mt19937& rng) {
    std::swap(a[lo], a[choosePivot(a, lo, hi, rule, cmp, rng)]);
    T pv = a[lo];
    long i = lo;
    for (long j = lo + 1; j <= hi; ++j)
        if (cmp(a[j], pv)) std::swap(a[++i], a[j]);
    std::swap(a[lo], a[i]);
    return i;
}

template <class T, class Cmp>
void quickRec(std::vector<T>& a, long lo, long hi, Pivot rule,
              Cmp cmp, std::mt19937& rng) {
    while (lo < hi) {
        long p = partitionLomuto(a, lo, hi, rule, cmp, rng);
        // recurse into the smaller side, loop on the larger side:
        // this keeps the recursion depth at O(log n) even on bad inputs
        if (p - lo < hi - p) { quickRec(a, lo, p - 1, rule, cmp, rng); lo = p + 1; }
        else                 { quickRec(a, p + 1, hi, rule, cmp, rng); hi = p - 1; }
    }
}

} // namespace detail

template <class T, class Cmp>
void quickSort(std::vector<T>& a, Pivot rule, Cmp cmp) {
    std::mt19937 rng(12345);
    detail::quickRec(a, 0, static_cast<long>(a.size()) - 1, rule, cmp, rng);
}