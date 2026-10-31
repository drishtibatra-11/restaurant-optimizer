#include "csv_loader.hpp"
#include "sort.hpp"
#include "test.hpp"
#include <algorithm>
#include <functional>
#include <random>
#include <string>
#include <vector>

static std::vector<int> makeData(const std::string& kind, int n) {
    std::mt19937 rng(7);
    std::vector<int> v(n);
    for (int& x : v) x = static_cast<int>(rng() % 1000);
    if (kind == "sorted")   std::sort(v.begin(), v.end());
    if (kind == "reversed") std::sort(v.rbegin(), v.rend());
    if (kind == "equal")    std::fill(v.begin(), v.end(), 5);
    return v;
}

// true if every sort gives the same answer as std::sort
static bool allSortsCorrect(const std::vector<int>& in) {
    std::vector<int> expect = in;
    std::sort(expect.begin(), expect.end());

    std::vector<int> a = in;
    mergeSort(a, std::less<int>());
    if (a != expect) return false;

    for (Pivot p : {Pivot::First, Pivot::Random, Pivot::Median3}) {
        a = in;
        quickSort(a, p, std::less<int>());
        if (a != expect) return false;
    }
    return true;
}

struct Item { int key; int idx; };

int main() {
    // edge cases
    CHECK(allSortsCorrect({}));
    CHECK(allSortsCorrect({42}));
    CHECK(allSortsCorrect({2, 1}));
    CHECK(allSortsCorrect({3, 3, 3}));

    // different shapes and sizes
    for (int n : {10, 100, 2000}) {
        CHECK(allSortsCorrect(makeData("random", n)));
        CHECK(allSortsCorrect(makeData("sorted", n)));
        CHECK(allSortsCorrect(makeData("reversed", n)));
        CHECK(allSortsCorrect(makeData("equal", n)));
    }

    // merge sort must be stable: equal keys keep their original order
    {
        std::vector<Item> v;
        for (int i = 0; i < 500; ++i) v.push_back({i % 7, i});
        mergeSort(v, [](const Item& a, const Item& b) { return a.key < b.key; });
        bool stable = true;
        for (std::size_t i = 1; i < v.size(); ++i) {
            if (v[i - 1].key > v[i].key) stable = false;
            if (v[i - 1].key == v[i].key && v[i - 1].idx > v[i].idx) stable = false;
        }
        CHECK(stable);
    }

    // sort real reservations by start time (uses the Week 1 loader)
    {
        Dataset d;
        std::string err;
        if (loadDataset("data/medium", d, err)) {
            auto byStart = [](const Reservation& a, const Reservation& b) {
                return a.start < b.start;
            };
            std::vector<Reservation> a = d.reservations;
            mergeSort(a, byStart);
            CHECK(std::is_sorted(a.begin(), a.end(), byStart));
            a = d.reservations;
            quickSort(a, Pivot::Median3, byStart);
            CHECK(std::is_sorted(a.begin(), a.end(), byStart));
        } else {
            CHECK(false);  // run "make data" first
        }
    }

    return TEST_SUMMARY();
}