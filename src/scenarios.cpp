#include "scenarios.hpp"
#include "bnb.hpp"
#include "csv_loader.hpp"
#include "dp.hpp"
#include "greedy.hpp"
#include "graph.hpp"
#include "huffman.hpp"
#include "sim.hpp"
#include "sort.hpp"
#include "strmatch.hpp"
#include "timer.hpp"
#include <algorithm>
#include <iomanip>
#include <ostream>
#include <vector>

namespace {

struct Rep {
    ResultTable table;
    std::ostream& out;
    int scenario;
    Rep(std::ostream& o, int s) : out(o), scenario(s) {
        table.setColumns({"scenario", "section", "strategy", "metric", "value"});
    }
    void add(const std::string& section, const std::string& strategy, const std::string& metric,
             double value, int digits = 2) {
        table.addRow({std::to_string(scenario), section, strategy, metric, fmt(value, digits)});
    }
    void addText(const std::string& section, const std::string& strategy, const std::string& metric,
                 const std::string& value) {
        table.addRow({std::to_string(scenario), section, strategy, metric, value});
    }
    void heading(const std::string& s) { out << "\n--- " << s << " ---\n"; }
};

const SeatPolicy kSeatPolicies[] = {SeatPolicy::FirstFit, SeatPolicy::EarliestFit, SeatPolicy::TightFit};
const KitchenPolicy kKitchenPolicies[] = {KitchenPolicy::FIFO, KitchenPolicy::ShortestFirst};
const Strategy kBnb[] = {Strategy::FIFO, Strategy::LIFO, Strategy::LeastCost};

void seatingSection(Rep& r, const std::string& tag, const std::vector<Reservation>& res,
                    const std::vector<Table>& tables) {
    for (SeatPolicy p : kSeatPolicies) {
        SeatStats s = simulateSeating(res, tables, p, 30);
        std::string name = std::string(seatPolicyName(p)) + tag;
        r.add("seating", name, "seated", static_cast<double>(s.seated), 0);
        r.add("seating", name, "lost", static_cast<double>(s.lost), 0);
        r.add("seating", name, "avg_wait_min", s.avgWait);
        r.add("seating", name, "max_wait_min", s.maxWait, 0);
        r.add("seating", name, "wasted_seat_minutes", static_cast<double>(s.wastedSeatMinutes), 0);
        r.add("seating", name, "utilization", s.utilization, 3);
        r.add("seating", name, "time_ms", s.ms, 3);
        r.out << "  " << std::left << std::setw(22) << name << std::right << "seated " << std::setw(6) << s.seated
              << "  lost " << std::setw(6) << s.lost << "  avg wait " << std::setw(6) << fmt(s.avgWait, 1)
              << " min  wasted seat-min " << std::setw(8) << s.wastedSeatMinutes << "  util "
              << fmt(s.utilization, 2) << "  " << fmt(s.ms, 1) << " ms\n";
    }
}

void kitchenSection(Rep& r, const std::string& tag, const Dataset& d) {
    for (KitchenPolicy p : kKitchenPolicies) {
        KitchenStats k = simulateKitchen(d, p);
        std::string name = std::string(kitchenPolicyName(p)) + tag;
        r.add("kitchen", name, "avg_ticket_min", k.avgTicket);
        r.add("kitchen", name, "max_ticket_min", k.maxTicket, 0);
        r.add("kitchen", name, "avg_item_wait_min", k.avgItemWait);
        r.add("kitchen", name, "utilization", k.avgUtilization, 3);
        r.add("kitchen", name, "time_ms", k.ms, 3);
        r.out << "  " << std::left << std::setw(24) << name << std::right << "avg ticket " << std::setw(8)
              << fmt(k.avgTicket, 1) << " min  max ticket " << std::setw(6) << k.maxTicket << "  avg item wait "
              << std::setw(8) << fmt(k.avgItemWait, 1) << "  util " << fmt(k.avgUtilization, 2) << "  "
              << fmt(k.ms, 1) << " ms\n";
    }
}

// up to `limit` parties that book between 18:00 and 20:00
std::vector<Reservation> rushParties(const std::vector<Reservation>& all, std::size_t limit) {
    std::vector<Reservation> rush;
    for (const Reservation& x : all)
        if (x.start >= 1080 && x.start < 1200 && rush.size() < limit) rush.push_back(x);
    return rush;
}

void exactSection(Rep& r, const std::string& tag, const std::vector<Reservation>& rush,
                  const std::vector<Table>& tables) {
    if (rush.empty() || tables.empty()) { r.out << "  (no rush-hour parties to assign)\n"; return; }
    CostMatrix c = seatingCosts(rush, tables, kitchenWalk(tables));
    AssignResult dp = assignDP(c), gr = assignGreedy(c);
    r.out << "  " << rush.size() << " parties, " << tables.size() << " tables. ";
    if (!dp.feasible) {
        r.addText("exact_seating", "DP" + tag, "result", "no valid seating");
        r.out << "No valid seating exists.\n";
        return;
    }
    r.add("exact_seating", "DP" + tag, "cost", static_cast<double>(dp.cost), 0);
    r.add("exact_seating", "DP" + tag, "time_ms", dp.ms, 3);
    r.out << "Optimal cost " << dp.cost << " (DP " << fmt(dp.ms, 2) << " ms)";
    if (gr.feasible) {
        r.add("exact_seating", "Greedy" + tag, "cost", static_cast<double>(gr.cost), 0);
        r.out << ", greedy " << gr.cost;
    }
    r.out << "\n";
    for (Strategy s : kBnb) {
        AssignResult b = branchAndBound(c, s);
        std::string name = std::string("BnB-") + strategyName(s) + tag;
        r.add("exact_seating", name, "cost", static_cast<double>(b.cost), 0);
        r.add("exact_seating", name, "nodes_expanded", static_cast<double>(b.expanded), 0);
        r.add("exact_seating", name, "peak_frontier", static_cast<double>(b.peakFrontier), 0);
        r.add("exact_seating", name, "time_ms", b.ms, 3);
        r.out << "    " << std::left << std::setw(20) << name << std::right << "cost " << std::setw(5) << b.cost
              << "  expanded " << std::setw(8) << b.expanded << "  peak frontier " << std::setw(8)
              << b.peakFrontier << "  " << fmt(b.ms, 2) << " ms\n";
    }
}

bool load(const std::string& dir, Dataset& d, std::ostream& out) {
    std::string err;
    if (!loadDataset(dir, d, err)) { out << "ERROR: " << err << "\n"; return false; }
    out << "Dataset '" << dir << "': " << d.reservations.size() << " reservations, " << d.tables.size()
        << " tables, " << d.orders.size() << " orders\n";
    return true;
}

// The generated sample data books far more guests than 20 tables and a few cooks can serve.
// For the "normal evening" and "shock" scenarios the restaurant is staffed to its demand:
// the table list is repeated until there are about 1.2 times as many tables as the bookings
// can occupy at once, and every station gets enough cooks for roughly 80% load.
Dataset sizedForDemand(const Dataset& d, Rep& r, bool scaleTables = true) {
    Dataset out = d;
    std::size_t copies = 1;
    if (scaleTables) {
        int need = intervalPartition(d.reservations).resources;
        while (copies * d.tables.size() < static_cast<std::size_t>(need) * 12 / 10) ++copies;
    }
    int maxId = 0;
    for (const Table& t : d.tables) maxId = std::max(maxId, t.id);
    out.tables.clear();
    for (std::size_t c = 0; c < copies; ++c)
        for (const Table& t : d.tables) out.tables.push_back({t.id + static_cast<int>(c) * maxId, t.seats, t.zone});

    KitchenStats k = simulateKitchen(d, KitchenPolicy::FIFO);
    long long busy = 0;
    int lo = 1 << 30, hi = 0;
    for (const Order& o : d.orders) { lo = std::min(lo, o.placedAt); hi = std::max(hi, o.placedAt); }
    for (const KitchenSlot& sl : k.slots) busy += sl.finish - sl.start;
    long long capTotal = 0;
    for (const Station& st : d.stations) capTotal += st.capacity;
    long long span = std::max(60, hi - lo + 60);
    long long factor = 1;
    if (capTotal > 0)
        factor = std::max<long long>(1, (busy * 10 + span * capTotal * 8 - 1) / (span * capTotal * 8));
    for (Station& st : out.stations) st.capacity = static_cast<int>(st.capacity * factor);
    r.out << "Demo sizing: tables x" << copies << " (" << out.tables.size() << " tables), cooks x" << factor
          << " per station, so the restaurant is busy but not hopeless\n";
    r.add("setup", "demo_sizing", "table_copies", static_cast<double>(copies), 0);
    r.add("setup", "demo_sizing", "cook_factor", static_cast<double>(factor), 0);
    return out;
}

// ---------------- scenario 1 ----------------
void normal(Rep& r, const Dataset& d) {
    r.heading("Seating: three table-choice strategies, same bookings (patience 30 min)");
    seatingSection(r, "", d.reservations, d.tables);
    PartitionResult p = intervalPartition(d.reservations);
    r.add("seating", "IntervalPartitioning", "min_tables_ignoring_size", p.resources, 0);
    r.out << "  Interval partitioning: " << p.resources << " tables would be enough if seat counts did not matter ("
          << d.tables.size() << " available)\n";

    r.heading("Kitchen: FIFO vs shortest-first");
    kitchenSection(r, "", d);

    r.heading("Routing: Dijkstra vs Floyd-Warshall (kitchen to every table)");
    Graph plan = buildFloorPlan(d.tables);
    Timer t1;
    ShortestPaths dj = dijkstra(plan, 0);
    double msDj = t1.ms();
    long long far = 0;
    for (long long x : dj.dist) far = std::max(far, x);
    r.add("routing", "Dijkstra", "time_ms", msDj, 3);
    r.add("routing", "Dijkstra", "farthest_table", static_cast<double>(far), 0);
    r.out << "  Dijkstra        " << fmt(msDj, 3) << " ms, farthest table " << far << " steps away\n";
    if (plan.size() <= 400) {
        Timer t2;
        FWResult fw = floydWarshall(plan);
        double msFw = t2.ms();
        bool same = fw.dist[0] == dj.dist;
        r.add("routing", "FloydWarshall", "time_ms", msFw, 3);
        r.add("routing", "FloydWarshall", "same_answer_as_dijkstra", same ? 1 : 0, 0);
        r.out << "  Floyd-Warshall  " << fmt(msFw, 3) << " ms, " << (same ? "same distances" : "DIFFERENT distances")
              << " (it also gives all " << plan.size() * plan.size() << " pairs)\n";
    } else {
        r.out << "  Floyd-Warshall skipped: " << plan.size() << " nodes would need " << plan.size() << "^3 steps\n";
    }

    r.heading("Exact seating of the 18:00-20:00 rush: DP vs branch and bound");
    std::vector<Reservation> rush = rushParties(d.reservations, 6);
    std::vector<Table> tables(d.tables.begin(), d.tables.begin() + std::min<std::size_t>(10, d.tables.size()));
    exactSection(r, "", rush, tables);
}



// ---------------- scenario 2 ----------------
void shock(Rep& r, const Dataset& d) {
    std::size_t k = (d.tables.size() + 2) / 3;
    Dataset broken = removeLargestTables(d, k);
    int station = busiestStation(d);
    int cooks = 1;
    for (const Station& st : d.stations) if (st.id == station) cooks = st.capacity;
    int lost = std::max(1, cooks / 2);
    Dataset slow = reduceStation(broken, station, lost);
    r.out << "Shock: the " << k << " largest tables break, and station " << station << " loses " << lost
          << " of its " << cooks << " cooks\n";
    r.add("shock", "setup", "tables_before", static_cast<double>(d.tables.size()), 0);
    r.add("shock", "setup", "tables_after", static_cast<double>(broken.tables.size()), 0);
    r.add("shock", "setup", "station_losing_cooks", station, 0);
    r.add("shock", "setup", "cooks_lost", lost, 0);

    r.heading("Seating BEFORE the shock");
    seatingSection(r, " (before)", d.reservations, d.tables);
    r.heading("Seating AFTER the shock");
    seatingSection(r, " (after)", d.reservations, broken.tables);

    r.heading("Kitchen BEFORE the shock");
    kitchenSection(r, " (before)", d);
    r.heading("Kitchen AFTER the shock");
    kitchenSection(r, " (after)", slow);

    r.heading("Seat capacity as a flow (guests booked for 18:30 that can sit at once)");
    std::vector<Reservation> evening;
    long long guests = 0;
    for (const Reservation& x : d.reservations)
        if (x.start <= 1110 && x.end() > 1110) { evening.push_back(x); guests += x.party; }
    Timer t1;
    long long before = maxSeatableGuests(evening, d.tables);
    long long after = maxSeatableGuests(evening, broken.tables);
    r.add("capacity_flow", "MaxFlow", "guests_booked", static_cast<double>(guests), 0);
    r.add("capacity_flow", "MaxFlow", "guests_seatable_before", static_cast<double>(before), 0);
    r.add("capacity_flow", "MaxFlow", "guests_seatable_after", static_cast<double>(after), 0);
    r.add("capacity_flow", "MaxFlow", "time_ms", t1.ms(), 3);
    r.out << "  " << guests << " guests booked; " << before << " can be seated before, " << after << " after\n";

    r.heading("Exact re-seating of the rush after the shock: DP vs branch and bound");
    std::vector<Reservation> rush = rushParties(d.reservations, 6);
    std::vector<Table> first10(d.tables.begin(), d.tables.begin() + std::min<std::size_t>(10, d.tables.size()));
    Dataset small = d;
    small.tables = first10;
    Dataset smallBroken = removeLargestTables(small, 3);
    r.out << "  before:\n";
    exactSection(r, " (before)", rush, small.tables);
    r.out << "  after (3 largest of these 10 tables gone):\n";
    exactSection(r, " (after)", rush, smallBroken.tables);
}

// ---------------- scenario 3 ----------------
void stress(Rep& r, const Dataset& d) {
    r.heading("Sorting the reservations by start time");
    auto byStart = [](const Reservation& a, const Reservation& b) { return a.start < b.start; };
    {
        std::vector<Reservation> v = d.reservations;
        Timer t;
        mergeSort(v, byStart);
        double ms = t.ms();
        bool ok = std::is_sorted(v.begin(), v.end(), byStart);
        r.add("sorting", "MergeSort", "time_ms", ms, 3);
        r.add("sorting", "MergeSort", "sorted_ok", ok ? 1 : 0, 0);
        r.out << "  merge sort         " << fmt(ms, 2) << " ms, sorted=" << (ok ? "yes" : "NO") << "\n";
        v = d.reservations;
        t.reset();
        quickSort(v, Pivot::Median3, byStart);
        ms = t.ms();
        ok = std::is_sorted(v.begin(), v.end(), byStart);
        r.add("sorting", "QuickSort-Median3", "time_ms", ms, 3);
        r.add("sorting", "QuickSort-Median3", "sorted_ok", ok ? 1 : 0, 0);
        r.out << "  quick sort (median3) " << fmt(ms, 2) << " ms, sorted=" << (ok ? "yes" : "NO") << "\n";
    }

    r.heading("Searching the event log for \"SEATED id=9\"");
    std::string log = buildEventLog(d);
    r.out << "  log size: " << log.size() << " bytes\n";
    struct M { const char* name; Positions (*fn)(const std::string&, const std::string&, long long*); };
    const M matchers[] = {{"Naive", matchNaive}, {"RabinKarp", matchRabinKarp},
                          {"Automaton", matchAutomaton}, {"KMP", matchKMP}};
    std::size_t first = 0;
    for (const M& m : matchers) {
        long long work = 0;
        Timer t;
        Positions p = m.fn(log, "SEATED id=9", &work);
        double ms = t.ms();
        if (m.name == matchers[0].name) first = p.size();
        r.add("string_matching", m.name, "matches", static_cast<double>(p.size()), 0);
        r.add("string_matching", m.name, "work", static_cast<double>(work), 0);
        r.add("string_matching", m.name, "time_ms", ms, 3);
        r.out << "  " << std::left << std::setw(11) << m.name << std::right << p.size() << " matches"
              << (p.size() == first ? "" : "  (DIFFERENT!)") << ", work " << std::setw(10) << work << ", "
              << fmt(ms, 2) << " ms\n";
    }

    r.heading("Menu specials: knapsack DP vs greedy (1000 dishes, 5000 prep-minutes)");
    std::vector<KnapItem> items;
    for (int i = 0; i < 1000; ++i) {
        const Dish& x = d.dishes[static_cast<std::size_t>(i) % d.dishes.size()];
        items.push_back({x.prepTime + i % 3, static_cast<long long>(x.profit * 100) + (i * 37) % 50});
    }
    {
        Timer t;
        KnapResult dp = knapsackDP(items, 5000);
        double msDp = t.ms();
        t.reset();
        long long row = knapsackValueOnly(items, 5000);
        double msRow = t.ms();
        t.reset();
        KnapResult gr = knapsackGreedy(items, 5000);
        double msGr = t.ms();
        r.add("knapsack", "DP-table", "value", static_cast<double>(dp.value), 0);
        r.add("knapsack", "DP-table", "time_ms", msDp, 3);
        r.add("knapsack", "DP-one-row", "value", static_cast<double>(row), 0);
        r.add("knapsack", "DP-one-row", "time_ms", msRow, 3);
        r.add("knapsack", "Greedy", "value", static_cast<double>(gr.value), 0);
        r.add("knapsack", "Greedy", "time_ms", msGr, 3);
        r.out << "  DP table   value " << dp.value << " (" << fmt(msDp, 1) << " ms)\n"
              << "  DP one row value " << row << " (" << fmt(msRow, 1) << " ms)\n"
              << "  Greedy     value " << gr.value << " (" << fmt(msGr, 2) << " ms), "
              << fmt(100.0 * static_cast<double>(gr.value) / static_cast<double>(std::max<long long>(1, dp.value)), 1)
              << "% of optimal\n";
    }

    r.heading("Seating the whole day (patience 30 min)");
    seatingSection(r, "", d.reservations, d.tables);

    r.heading("Kitchen for the whole day");
    kitchenSection(r, "", sizedForDemand(d, r, false));
}

} // namespace

std::string defaultScenarioData(int which) { return which == 3 ? "data/large" : "data/medium"; }

ResultTable runScenario(int which, const std::string& dataDir, std::ostream& out) {
    Rep r(out, which);
    if (which < 1 || which > 3) { out << "Scenario must be 1, 2 or 3.\n"; return r.table; }
    static const char* names[] = {"", "Scenario 1: normal evening", "Scenario 2: capacity shock",
                                  "Scenario 3: large-data stress"};
    out << "\n=== " << names[which] << " ===\n";
    Dataset d;
    if (!load(dataDir, d, out)) return ResultTable();
    if (d.tables.empty() || d.stations.empty()) { out << "ERROR: dataset has no tables or stations\n"; return ResultTable(); }
    if (which == 1) normal(r, sizedForDemand(d, r));
    else if (which == 2) shock(r, sizedForDemand(d, r));
    else stress(r, d);
    return r.table;
}