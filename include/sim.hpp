#pragma once
#include "flow.hpp"
#include "model.hpp"
#include "sort.hpp"
#include "timer.hpp"
#include <algorithm>
#include <climits>
#include <cstddef>
#include <numeric>
#include <queue>
#include <unordered_map>
#include <utility>
#include <vector>

// =====================================================================
// Restaurant simulator: table seating over a day, the kitchen, and
// capacity shocks. All times are minutes since midnight.
// =====================================================================

// ---------- Seating ----------
// Reservations are handled in start-time order. A party is seated at a table with enough
// seats as soon as that table is free, but it leaves ("lost") if it would wait longer
// than `patience` minutes. The policies differ only in WHICH table they pick:
//   FirstFit    the first table (in list order) that fits and is free soon enough
//   EarliestFit the table with the shortest wait; ties: the fewest empty seats
//   TightFit    the table with the fewest empty seats; ties: the shortest wait
enum class SeatPolicy { FirstFit, EarliestFit, TightFit };

inline const char* seatPolicyName(SeatPolicy p) {
    return p == SeatPolicy::FirstFit ? "FirstFit" : p == SeatPolicy::EarliestFit ? "EarliestFit" : "TightFit";
}

struct SeatStats {
    std::size_t total = 0, seated = 0, lost = 0;
    double avgWait = 0;                   // minutes, over the seated parties
    int maxWait = 0;
    long long wastedSeatMinutes = 0;      // (seats - party) * duration, summed
    double utilization = 0;               // table-minutes in use / table-minutes available
    double ms = 0;                        // running time of the simulation
    std::vector<int> tableOf;             // per reservation (input order): table index, or -1 if lost
    std::vector<int> seatAt;              // per reservation: time seated, or -1
};

// Theta(n log n + n * tables)
inline SeatStats simulateSeating(const std::vector<Reservation>& res, const std::vector<Table>& tables,
                                 SeatPolicy pol, int patience = 30) {
    Timer tm;
    SeatStats st;
    st.total = res.size();
    st.tableOf.assign(res.size(), -1);
    st.seatAt.assign(res.size(), -1);
    std::vector<std::size_t> order(res.size());
    std::iota(order.begin(), order.end(), 0);
    mergeSort(order, [&](std::size_t a, std::size_t b) { return res[a].start < res[b].start; });

    std::vector<int> freeAt(tables.size(), 0);
    long long waitSum = 0, usedMinutes = 0;
    int minStart = INT_MAX, maxEnd = 0;
    for (std::size_t i : order) {
        const Reservation& r = res[i];
        int pick = -1, pickWait = 0, pickWaste = 0;
        for (std::size_t t = 0; t < tables.size(); ++t) {
            if (tables[t].seats < r.party) continue;
            int wait = std::max(r.start, freeAt[t]) - r.start;
            if (wait > patience) continue;
            int waste = tables[t].seats - r.party;
            bool better;
            if (pick < 0) better = true;
            else if (pol == SeatPolicy::FirstFit) better = false;
            else if (pol == SeatPolicy::EarliestFit) better = wait < pickWait || (wait == pickWait && waste < pickWaste);
            else better = waste < pickWaste || (waste == pickWaste && wait < pickWait);
            if (better) { pick = static_cast<int>(t); pickWait = wait; pickWaste = waste; }
            if (pol == SeatPolicy::FirstFit) break;
        }
        if (pick < 0) { ++st.lost; continue; }
        int seat = r.start + pickWait;
        freeAt[pick] = seat + r.dur;
        st.tableOf[i] = pick;
        st.seatAt[i] = seat;
        ++st.seated;
        waitSum += pickWait;
        st.maxWait = std::max(st.maxWait, pickWait);
        st.wastedSeatMinutes += static_cast<long long>(pickWaste) * r.dur;
        usedMinutes += r.dur;
        minStart = std::min(minStart, seat);
        maxEnd = std::max(maxEnd, seat + r.dur);
    }
    if (st.seated > 0) {
        st.avgWait = static_cast<double>(waitSum) / static_cast<double>(st.seated);
        long long span = static_cast<long long>(maxEnd) - minStart;
        if (span > 0 && !tables.empty())
            st.utilization = static_cast<double>(usedMinutes) / (static_cast<double>(tables.size()) * span);
    }
    st.ms = tm.ms();
    return st;
}

// ---------- Kitchen ----------
// Every dish of an order goes to its station when the order is placed. A station cooks as
// many dishes at once as its capacity says. Policies:
//   FIFO           first come, first served
//   ShortestFirst  when a cook is free, take the waiting dish with the shortest prep time
enum class KitchenPolicy { FIFO, ShortestFirst };

inline const char* kitchenPolicyName(KitchenPolicy p) {
    return p == KitchenPolicy::FIFO ? "FIFO" : "ShortestFirst";
}

struct KitchenSlot {
    int station;     // index into Dataset::stations
    int arrival;     // when the order was placed
    int start;
    int finish;
};

struct KitchenStats {
    std::size_t items = 0, orders = 0;
    double avgTicket = 0;                 // order placed -> last dish finished, in minutes
    int maxTicket = 0;
    double avgItemWait = 0;               // dish waiting for a free cook, in minutes
    int maxItemWait = 0;
    double avgUtilization = 0;            // cook-minutes used / cook-minutes available
    int makespan = 0;                     // time the last dish finishes
    double ms = 0;
    std::vector<KitchenSlot> slots;       // one per dish
};

// Theta(items log items)
inline KitchenStats simulateKitchen(const Dataset& d, KitchenPolicy pol) {
    Timer tm;
    KitchenStats ks;
    ks.orders = d.orders.size();
    std::unordered_map<int, std::size_t> dishIdx, stationIdx;
    for (std::size_t i = 0; i < d.dishes.size(); ++i) dishIdx[d.dishes[i].id] = i;
    for (std::size_t i = 0; i < d.stations.size(); ++i) stationIdx[d.stations[i].id] = i;

    struct Item { int arrival, prep, order, station; };
    std::vector<Item> items;
    for (std::size_t o = 0; o < d.orders.size(); ++o)
        for (int dishId : d.orders[o].dishes) {
            auto di = dishIdx.find(dishId);
            if (di == dishIdx.end()) continue;
            const Dish& dish = d.dishes[di->second];
            auto si = stationIdx.find(dish.station);
            if (si == stationIdx.end()) continue;
            items.push_back({d.orders[o].placedAt, dish.prepTime, static_cast<int>(o), static_cast<int>(si->second)});
        }
    ks.items = items.size();
    ks.slots.assign(items.size(), {0, 0, 0, 0});

    std::vector<std::vector<std::size_t>> perStation(d.stations.size());
    for (std::size_t k = 0; k < items.size(); ++k) perStation[items[k].station].push_back(k);

    for (std::size_t s = 0; s < perStation.size(); ++s) {
        std::vector<std::size_t>& list = perStation[s];
        mergeSort(list, [&](std::size_t a, std::size_t b) { return items[a].arrival < items[b].arrival; });
        std::priority_queue<int, std::vector<int>, std::greater<int>> cooks;   // times they become free
        for (int c = 0; c < d.stations[s].capacity; ++c) cooks.push(0);
        auto run = [&](std::size_t k, int start) {
            int finish = start + items[k].prep;
            ks.slots[k] = {static_cast<int>(s), items[k].arrival, start, finish};
            cooks.pop();
            cooks.push(finish);
        };
        if (cooks.empty()) continue;
        if (pol == KitchenPolicy::FIFO) {
            for (std::size_t k : list) run(k, std::max(cooks.top(), items[k].arrival));
        } else {
            using W = std::pair<int, std::size_t>;                              // (prep time, item)
            std::priority_queue<W, std::vector<W>, std::greater<W>> waiting;
            std::size_t next = 0;
            int now = 0;                                                        // the clock never runs backwards
            while (next < list.size() || !waiting.empty()) {
                int t = std::max(cooks.top(), now);
                if (waiting.empty()) t = std::max(t, items[list[next]].arrival);
                now = t;
                while (next < list.size() && items[list[next]].arrival <= t) {
                    waiting.push({items[list[next]].prep, list[next]});
                    ++next;
                }
                std::size_t k = waiting.top().second;
                waiting.pop();
                run(k, t);
            }
        }
    }

    std::vector<int> orderDone(d.orders.size(), 0);
    long long waitSum = 0;
    std::vector<long long> busy(d.stations.size(), 0);
    for (std::size_t k = 0; k < items.size(); ++k) {
        const KitchenSlot& sl = ks.slots[k];
        orderDone[items[k].order] = std::max(orderDone[items[k].order], sl.finish);
        waitSum += sl.start - sl.arrival;
        ks.maxItemWait = std::max(ks.maxItemWait, sl.start - sl.arrival);
        ks.makespan = std::max(ks.makespan, sl.finish);
        busy[items[k].station] += items[k].prep;
    }
    long long ticketSum = 0;
    std::size_t counted = 0;
    for (std::size_t o = 0; o < d.orders.size(); ++o) {
        if (orderDone[o] == 0) continue;
        int t = orderDone[o] - d.orders[o].placedAt;
        ticketSum += t;
        ks.maxTicket = std::max(ks.maxTicket, t);
        ++counted;
    }
    if (counted) ks.avgTicket = static_cast<double>(ticketSum) / static_cast<double>(counted);
    if (!items.empty()) ks.avgItemWait = static_cast<double>(waitSum) / static_cast<double>(items.size());
    if (ks.makespan > 0 && !d.stations.empty()) {
        double u = 0;
        for (std::size_t s = 0; s < d.stations.size(); ++s)
            u += static_cast<double>(busy[s]) / (static_cast<double>(d.stations[s].capacity) * ks.makespan);
        ks.avgUtilization = u / static_cast<double>(d.stations.size());
    }
    ks.ms = tm.ms();
    return ks;
}

// ---------- Capacity shocks ----------

// A copy of the dataset without its k largest tables (a worst-case breakdown).
inline Dataset removeLargestTables(const Dataset& d, std::size_t k) {
    Dataset out = d;
    std::vector<std::size_t> idx(d.tables.size());
    std::iota(idx.begin(), idx.end(), 0);
    mergeSort(idx, [&](std::size_t a, std::size_t b) {
        if (d.tables[a].seats != d.tables[b].seats) return d.tables[a].seats > d.tables[b].seats;
        return d.tables[a].id > d.tables[b].id;
    });
    std::vector<bool> drop(d.tables.size(), false);
    for (std::size_t i = 0; i < k && i < idx.size(); ++i) drop[idx[i]] = true;
    out.tables.clear();
    for (std::size_t i = 0; i < d.tables.size(); ++i) if (!drop[i]) out.tables.push_back(d.tables[i]);
    return out;
}

// A copy of the dataset where one station has `by` fewer cooks (never fewer than 1).
inline Dataset reduceStation(const Dataset& d, int stationId, int by) {
    Dataset out = d;
    for (Station& s : out.stations)
        if (s.id == stationId) s.capacity = std::max(1, s.capacity - by);
    return out;
}

// Id of the station with the most prep-minutes per cook (the one a missing cook hurts most).
inline int busiestStation(const Dataset& d) {
    std::unordered_map<int, long long> load;
    for (const Dish& x : d.dishes) load[x.id] = x.prepTime;
    std::unordered_map<int, long long> minutes;
    std::unordered_map<int, int> stationOfDish;
    for (const Dish& x : d.dishes) stationOfDish[x.id] = x.station;
    for (const Order& o : d.orders)
        for (int id : o.dishes) minutes[stationOfDish[id]] += load[id];
    int best = d.stations.empty() ? -1 : d.stations[0].id;
    double bestLoad = -1;
    for (const Station& s : d.stations) {
        double l = static_cast<double>(minutes[s.id]) / s.capacity;
        if (l > bestLoad) { bestLoad = l; best = s.id; }
    }
    return best;
}

// Capacity allocation as a flow problem: how many guests can be seated at the same time?
// source -> party (capacity = party size) -> zone -> sink (capacity = seats in the zone).
// A party with a preferred zone may only use that zone; party with pref 0 may use any zone.
// (This counts seats, so it ignores that one party must sit at ONE table.)
inline long long maxSeatableGuests(const std::vector<Reservation>& res, const std::vector<Table>& tables) {
    std::vector<int> zones;
    for (const Table& t : tables) zones.push_back(t.zone);
    std::sort(zones.begin(), zones.end());
    zones.erase(std::unique(zones.begin(), zones.end()), zones.end());
    int n = static_cast<int>(res.size()), z = static_cast<int>(zones.size());
    FlowNetwork net(n + z + 2);
    int s = n + z, t = s + 1;
    std::vector<long long> seats(z, 0);
    for (const Table& tb : tables)
        seats[std::lower_bound(zones.begin(), zones.end(), tb.zone) - zones.begin()] += tb.seats;
    for (int i = 0; i < n; ++i) {
        net.addEdge(s, i, res[i].party);
        for (int k = 0; k < z; ++k)
            if (res[i].prefZone == 0 || res[i].prefZone == zones[k]) net.addEdge(i, n + k, res[i].party);
    }
    for (int k = 0; k < z; ++k) net.addEdge(n + k, t, seats[k]);
    return net.maxFlowBFS(s, t);
}