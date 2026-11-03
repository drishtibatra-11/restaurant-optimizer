// Benchmark runner: time, peak extra memory, solution quality and scalability.
// Writes results/bench_all.csv and results/bench_all.json.
//
// Memory is measured by replacing operator new/delete below: "peak_memory_kb" is the most
// heap memory that was in use at one moment while the algorithm ran, minus what was already
// in use before it started (so copies and tables made by the algorithm are counted).
#include "bnb.hpp"
#include "csv_loader.hpp"
#include "dp.hpp"
#include "graph.hpp"
#include "huffman.hpp"
#include "report.hpp"
#include "sim.hpp"
#include "sort.hpp"
#include "strmatch.hpp"
#include "timer.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <new>
#include <random>
#include <string>
#include <vector>

// ---------- heap tracking ----------
static std::size_t g_cur = 0, g_peak = 0;
static const std::size_t HEADER = 16;   // keeps the memory 16-byte aligned

void* operator new(std::size_t n) {
    void* p = std::malloc(n + HEADER);
    if (!p) throw std::bad_alloc();
    *static_cast<std::size_t*>(p) = n;
    g_cur += n;
    if (g_cur > g_peak) g_peak = g_cur;
    return static_cast<char*>(p) + HEADER;
}
void operator delete(void* p) noexcept {
    if (!p) return;
    char* base = static_cast<char*>(p) - HEADER;
    g_cur -= *reinterpret_cast<std::size_t*>(base);
    std::free(base);
}
void operator delete(void* p, std::size_t) noexcept { operator delete(p); }
void* operator new[](std::size_t n) { return operator new(n); }
void operator delete[](void* p) noexcept { operator delete(p); }
void operator delete[](void* p, std::size_t) noexcept { operator delete(p); }

struct Measured { double ms; double peakKB; };

template <class F>
Measured measure(F f) {
    std::size_t base = g_cur;
    g_peak = g_cur;
    Timer t;
    f();
    double ms = t.ms();
    return {ms, static_cast<double>(g_peak - base) / 1024.0};
}

// ---------- output ----------
static ResultTable g_table;

static void emit(const std::string& task, const std::string& dataset, std::size_t n, const std::string& algo,
                 const Measured& m, const std::string& metric, double value) {
    g_table.addRow({task, dataset, std::to_string(n), algo, fmt(m.ms, 3), fmt(m.peakKB, 1), metric, fmt(value, 2)});
    std::cout << std::left << std::setw(10) << task << std::setw(14) << dataset << std::right << std::setw(8) << n
              << "  " << std::left << std::setw(18) << algo << std::right << std::setw(11) << fmt(m.ms, 2)
              << " ms " << std::setw(11) << fmt(m.peakKB, 0) << " KB   " << metric << " = " << fmt(value, 2) << "\n";
}

// ---------- tasks that run on a dataset ----------
static void benchDataset(const std::string& name, const Dataset& d) {
    const std::vector<Reservation>& res = d.reservations;
    auto byStart = [](const Reservation& a, const Reservation& b) { return a.start < b.start; };

    // sorting
    struct S { const char* name; int mode; };
    for (S s : {S{"merge", 0}, S{"quick_first", 1}, S{"quick_median3", 2}}) {
        bool ok = false;
        Measured m = measure([&] {
            std::vector<Reservation> v = res;
            if (s.mode == 0) mergeSort(v, byStart);
            else quickSort(v, s.mode == 1 ? Pivot::First : Pivot::Median3, byStart);
            ok = std::is_sorted(v.begin(), v.end(), byStart);
        });
        emit("sort", name, res.size(), s.name, m, "sorted_ok", ok ? 1 : 0);
    }

    // seating
    for (SeatPolicy p : {SeatPolicy::FirstFit, SeatPolicy::EarliestFit, SeatPolicy::TightFit}) {
        SeatStats st;
        Measured m = measure([&] { st = simulateSeating(res, d.tables, p, 30); });
        emit("seating", name, res.size(), seatPolicyName(p), m, "seated", static_cast<double>(st.seated));
        emit("seating", name, res.size(), seatPolicyName(p), m, "avg_wait_min", st.avgWait);
    }

    // kitchen
    for (KitchenPolicy p : {KitchenPolicy::FIFO, KitchenPolicy::ShortestFirst}) {
        KitchenStats ks;
        Measured m = measure([&] { ks = simulateKitchen(d, p); });
        emit("kitchen", name, d.orders.size(), kitchenPolicyName(p), m, "avg_item_wait_min", ks.avgItemWait);
    }

    // string matching on the event log
    std::string log = buildEventLog(d);
    struct M { const char* name; Positions (*fn)(const std::string&, const std::string&, long long*); };
    for (M mm : {M{"naive", matchNaive}, M{"rabin_karp", matchRabinKarp},
                 M{"automaton", matchAutomaton}, M{"kmp", matchKMP}}) {
        std::size_t found = 0;
        Measured m = measure([&] { found = mm.fn(log, "SEATED id=9", nullptr).size(); });
        emit("strmatch", name, log.size(), mm.name, m, "matches", static_cast<double>(found));
    }

    // Huffman compression of the log
    {
        std::size_t packed = 0;
        Measured m = measure([&] {
            HuffmanTable t = buildHuffman(log);
            packed = packBits(huffmanEncode(log, t)).size();
        });
        emit("huffman", name, log.size(), "huffman", m, "compressed_percent",
             log.empty() ? 0.0 : 100.0 * static_cast<double>(packed) / static_cast<double>(log.size()));
    }

    // shortest paths on this floor plan: all pairs
    {
        Graph plan = buildFloorPlan(d.tables);
        long long sumDj = 0, sumFw = 0;
        Measured m1 = measure([&] {
            for (int u = 0; u < plan.size(); ++u)
                for (long long x : dijkstra(plan, u).dist) if (x < INF) sumDj += x;
        });
        emit("allpairs", name, static_cast<std::size_t>(plan.size()), "dijkstra_x_n", m1, "distance_sum", static_cast<double>(sumDj));
        if (plan.size() <= 400) {
            Measured m2 = measure([&] {
                FWResult fw = floydWarshall(plan);
                for (const auto& row : fw.dist) for (long long x : row) if (x < INF) sumFw += x;
            });
            emit("allpairs", name, static_cast<std::size_t>(plan.size()), "floyd_warshall", m2, "distance_sum", static_cast<double>(sumFw));
        }
    }
}

// ---------- tasks that do not depend on a dataset ----------
static void benchKnapsack() {
    struct Size { int n, cap; };
    for (Size sz : {Size{50, 250}, Size{200, 1000}, Size{1000, 5000}}) {
        std::mt19937 rng(7);
        std::vector<KnapItem> items;
        for (int i = 0; i < sz.n; ++i) items.push_back({1 + static_cast<int>(rng() % 40), 1 + static_cast<long long>(rng() % 100)});
        long long vDp = 0, vRow = 0, vGr = 0;
        Measured a = measure([&] { vDp = knapsackDP(items, sz.cap).value; });
        Measured b = measure([&] { vRow = knapsackValueOnly(items, sz.cap); });
        Measured c = measure([&] { vGr = knapsackGreedy(items, sz.cap).value; });
        std::string label = "n" + std::to_string(sz.n) + "_W" + std::to_string(sz.cap);
        emit("knapsack", label, static_cast<std::size_t>(sz.n), "dp_table", a, "value", static_cast<double>(vDp));
        emit("knapsack", label, static_cast<std::size_t>(sz.n), "dp_one_row", b, "value", static_cast<double>(vRow));
        emit("knapsack", label, static_cast<std::size_t>(sz.n), "greedy", c, "value", static_cast<double>(vGr));
    }
}

static void benchBranchAndBound() {
    std::mt19937 rng(132);
    CostMatrix c(6, std::vector<long long>(9, 0));
    for (auto& row : c) for (long long& x : row) x = 1 + static_cast<long long>(rng() % 30);
    for (Strategy s : {Strategy::FIFO, Strategy::LIFO, Strategy::LeastCost}) {
        AssignResult r;
        Measured m = measure([&] { r = branchAndBound(c, s); });
        emit("bnb", "6x9", 6, strategyName(s), m, "cost", static_cast<double>(r.cost));
        emit("bnb", "6x9", 6, strategyName(s), m, "nodes_expanded", static_cast<double>(r.expanded));
    }
    AssignResult d;
    Measured m1 = measure([&] { d = assignDP(c); });
    emit("bnb", "6x9", 6, "dp", m1, "cost", static_cast<double>(d.cost));
    AssignResult g;
    Measured m2 = measure([&] { g = assignGreedy(c); });
    emit("bnb", "6x9", 6, "greedy", m2, "cost", static_cast<double>(g.cost));
}

static void benchScalingPaths() {
    for (int n : {50, 100, 200, 400}) {
        std::vector<Table> tables;
        for (int i = 1; i <= n; ++i) tables.push_back({i, 4, 1});
        Graph plan = buildFloorPlan(tables);
        long long sum = 0;
        Measured a = measure([&] {
            for (int u = 0; u < plan.size(); ++u)
                for (long long x : dijkstra(plan, u).dist) if (x < INF) sum += x;
        });
        emit("allpairs", "floor-" + std::to_string(n), static_cast<std::size_t>(plan.size()), "dijkstra_x_n", a,
             "distance_sum", static_cast<double>(sum));
        long long sum2 = 0;
        Measured b = measure([&] {
            FWResult fw = floydWarshall(plan);
            for (const auto& row : fw.dist) for (long long x : row) if (x < INF) sum2 += x;
        });
        emit("allpairs", "floor-" + std::to_string(n), static_cast<std::size_t>(plan.size()), "floyd_warshall", b,
             "distance_sum", static_cast<double>(sum2));
    }
}

int main() {
    g_table.setColumns({"task", "dataset", "n", "algorithm", "time_ms", "peak_memory_kb", "quality_metric", "quality_value"});
    struct D { const char* name; const char* dir; };
    const D sets[] = {{"small", "data/small"}, {"medium", "data/medium"}, {"large", "data/large"},
                      {"adv-sorted", "data/adversarial/sorted"}, {"adv-reversed", "data/adversarial/reversed"},
                      {"adv-equal", "data/adversarial/equal"}};
    for (const D& s : sets) {
        Dataset d;
        std::string err;
        if (!loadDataset(s.dir, d, err)) { std::cerr << "skipping " << s.dir << ": " << err << " (run 'make data')\n"; continue; }
        std::cout << "\n== " << s.name << " ==\n";
        benchDataset(s.name, d);
    }
    std::cout << "\n== synthetic ==\n";
    benchKnapsack();
    benchBranchAndBound();
    benchScalingPaths();

    std::filesystem::create_directories("results");
    bool ok = g_table.save("results/bench_all.csv") && g_table.save("results/bench_all.json");
    if (!ok) { std::cerr << "cannot write results/bench_all.*\n"; return 1; }
    std::cout << "\nWrote results/bench_all.csv and results/bench_all.json (" << g_table.rows() << " rows)\n";
    return 0;
}