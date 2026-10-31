#include "csv_loader.hpp"
#include "sort.hpp"
#include "timer.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Runs merge sort and the 3 quick sort variants on one input and records the results.
template <class T, class Less>
void benchAll(const std::string& dataset, const std::string& kind,
              const std::vector<T>& input, Less less, std::ofstream& csv) {
    struct Algo { const char* name; int mode; const char* mem; };
    const Algo algos[] = {
        {"merge",         0, "O(n)"},
        {"quick_first",   1, "O(log n)"},
        {"quick_random",  2, "O(log n)"},
        {"quick_median3", 3, "O(log n)"}};
    const Pivot pivots[] = {Pivot::First, Pivot::First, Pivot::Random, Pivot::Median3};

    int reps = input.size() <= 10000 ? 3 : 1;
    for (const Algo& a : algos) {
        double best = 1e18;
        long long comps = 0;
        bool ok = true;
        for (int r = 0; r < reps; ++r) {
            std::vector<T> v = input;               // fresh copy, not timed
            long long c = 0;
            auto cmp = [&](const T& x, const T& y) { ++c; return less(x, y); };
            Timer t;
            if (a.mode == 0) mergeSort(v, cmp);
            else             quickSort(v, pivots[a.mode], cmp);
            double ms = t.ms();
            if (ms < best) best = ms;
            comps = c;
            ok = ok && std::is_sorted(v.begin(), v.end(), less);
        }
        csv << dataset << ',' << kind << ',' << input.size() << ',' << a.name << ','
            << std::fixed << std::setprecision(3) << best << ',' << comps << ','
            << a.mem << ',' << (ok ? "yes" : "NO") << '\n';
        std::cout << std::left << std::setw(10) << dataset << std::setw(10) << kind
                  << std::right << std::setw(8) << input.size() << "  "
                  << std::left << std::setw(15) << a.name << std::right
                  << std::setw(11) << std::fixed << std::setprecision(2) << best << " ms"
                  << std::setw(14) << comps << " cmp  sorted=" << (ok ? "yes" : "NO") << "\n";
    }
}

static std::vector<int> makeInts(const std::string& kind, int n) {
    std::mt19937 rng(7);
    std::vector<int> v(n);
    for (int& x : v) x = static_cast<int>(rng() % 1000000);
    if (kind == "sorted")   std::sort(v.begin(), v.end());
    if (kind == "reversed") std::sort(v.rbegin(), v.rend());
    if (kind == "equal")    std::fill(v.begin(), v.end(), 5);
    return v;
}

int main() {
    fs::create_directories("results");
    std::ofstream csv("results/bench_sort.csv");
    if (!csv) { std::cerr << "cannot write results/bench_sort.csv\n"; return 1; }
    csv << "dataset,kind,n,algorithm,time_ms,comparisons,extra_memory,sorted_ok\n";

    std::cout << "== Integer arrays (scaling) ==\n";
    for (const char* kind : {"random", "sorted", "reversed", "equal"})
        for (int n : {1000, 10000, 30000})
            benchAll("ints", kind, makeInts(kind, n), std::less<int>(), csv);

    std::cout << "\n== Real reservation datasets, sorted by start time ==\n";
    struct Real { const char* dir; const char* kind; };
    const Real reals[] = {{"data/large", "random"},
                          {"data/adversarial/sorted", "sorted"},
                          {"data/adversarial/reversed", "reversed"},
                          {"data/adversarial/equal", "equal"}};
    auto byStart = [](const Reservation& a, const Reservation& b) { return a.start < b.start; };
    for (const Real& r : reals) {
        Dataset d;
        std::string err;
        if (!loadDataset(r.dir, d, err)) {
            std::cerr << "skipping " << r.dir << ": " << err << " (run 'make data')\n";
            continue;
        }
        benchAll("reservations", r.kind, d.reservations, byStart, csv);
    }

    std::cout << "\nWrote results/bench_sort.csv\n";
    return 0;
}