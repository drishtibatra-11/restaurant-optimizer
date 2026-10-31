#include "csv_loader.hpp"
#include "search.hpp"
#include "sort.hpp"
#include "test.hpp"
#include <algorithm>
#include <functional>
#include <random>
#include <vector>

// Simple, obviously correct versions used as the reference
static long linearFind(const std::vector<int>& a, int t) {
    for (std::size_t i = 0; i < a.size(); ++i) if (a[i] == t) return static_cast<long>(i);
    return -1;
}
static std::size_t linearLower(const std::vector<int>& a, int t) {
    std::size_t i = 0;
    while (i < a.size() && a[i] < t) ++i;
    return i;
}

// The index may differ when there are duplicates, so check the value found.
static bool findOk(const std::vector<int>& a, int t, long idx) {
    if (linearFind(a, t) == -1) return idx == -1;
    return idx >= 0 && idx < static_cast<long>(a.size()) && a[idx] == t;
}

int main() {
    // edge cases
    std::vector<int> empty;
    CHECK(binarySearchIter(empty, 5) == -1);
    CHECK(binarySearchRec(empty, 5) == -1);
    CHECK(lowerBound(empty, 5) == 0);

    std::vector<int> one = {7};
    CHECK(binarySearchIter(one, 7) == 0);
    CHECK(binarySearchRec(one, 7) == 0);
    CHECK(binarySearchIter(one, 8) == -1);
    CHECK(binarySearchRec(one, 6) == -1);

    // random sorted arrays, every target from below the range to above it
    std::mt19937 rng(3);
    bool allOk = true;
    for (int n : {2, 5, 50, 500}) {
        std::vector<int> a(n);
        for (int& x : a) x = static_cast<int>(rng() % 100);
        mergeSort(a, std::less<int>());
        for (int t = -2; t <= 102; ++t) {
            if (!findOk(a, t, binarySearchIter(a, t))) allOk = false;
            if (!findOk(a, t, binarySearchRec(a, t))) allOk = false;
            if (lowerBound(a, t) != linearLower(a, t)) allOk = false;
        }
    }
    CHECK(allOk);

    // first table with at least n seats
    {
        std::vector<Table> t = {{1, 2, 1}, {2, 4, 1}, {3, 4, 2}, {4, 6, 2}, {5, 8, 3}};
        CHECK(firstTableAtLeast(t, 1) == 0);
        CHECK(firstTableAtLeast(t, 2) == 0);
        CHECK(firstTableAtLeast(t, 3) == 1);
        CHECK(firstTableAtLeast(t, 4) == 1);
        CHECK(firstTableAtLeast(t, 5) == 3);
        CHECK(firstTableAtLeast(t, 8) == 4);
        CHECK(firstTableAtLeast(t, 9) == -1);
        CHECK(firstTableAtLeast({}, 2) == -1);
    }

    // real data: sort the medium tables by seats, then search
    {
        Dataset d;
        std::string err;
        if (loadDataset("data/medium", d, err)) {
            std::vector<Table> t = d.tables;
            mergeSort(t, [](const Table& a, const Table& b) { return a.seats < b.seats; });
            long i = firstTableAtLeast(t, 5);
            CHECK(i >= 0 && t[i].seats >= 5 && (i == 0 || t[i - 1].seats < 5));
        } else {
            CHECK(false);  // run "make data" first
        }
    }

    return TEST_SUMMARY();
}