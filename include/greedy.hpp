#pragma once
#include "model.hpp"
#include "sort.hpp"
#include <cstddef>
#include <functional>
#include <queue>
#include <utility>
#include <vector>

// Reservations are half-open intervals [start, end).
inline bool overlaps(const Reservation& a, const Reservation& b) {
    return a.start < b.end() && b.start < a.end();
}

// ---------- Activity selection ----------
// Most non-overlapping reservations for one table. Greedy: earliest finish first.
// Returns indices into `r`, in time order. Theta(n log n).
inline std::vector<std::size_t> activitySelection(const std::vector<Reservation>& r) {
    std::vector<std::size_t> idx(r.size());
    for (std::size_t i = 0; i < idx.size(); ++i) idx[i] = i;
    mergeSort(idx, [&](std::size_t a, std::size_t b) { return r[a].end() < r[b].end(); });

    std::vector<std::size_t> chosen;
    int lastEnd = 0;
    for (std::size_t i : idx) {
        if (chosen.empty() || r[i].start >= lastEnd) {
            chosen.push_back(i);
            lastEnd = r[i].end();
        }
    }
    return chosen;
}

// ---------- Interval partitioning ----------
struct PartitionResult {
    int resources;            // minimum number of tables needed
    std::vector<int> assign;  // assign[i] = table number (0..resources-1) of reservation i
};

// Greedy: by start time, reuse the table that frees up first. Theta(n log n).
inline PartitionResult intervalPartition(const std::vector<Reservation>& r) {
    std::vector<std::size_t> idx(r.size());
    for (std::size_t i = 0; i < idx.size(); ++i) idx[i] = i;
    mergeSort(idx, [&](std::size_t a, std::size_t b) { return r[a].start < r[b].start; });

    using Busy = std::pair<int, int>;   // (time the table becomes free, table number)
    std::priority_queue<Busy, std::vector<Busy>, std::greater<Busy>> busy;

    PartitionResult res{0, std::vector<int>(r.size(), -1)};
    for (std::size_t i : idx) {
        int table;
        if (!busy.empty() && busy.top().first <= r[i].start) {
            table = busy.top().second;   // a table is already free: reuse it
            busy.pop();
        } else {
            table = res.resources++;     // none free: open a new table
        }
        res.assign[i] = table;
        busy.push({r[i].end(), table});
    }
    return res;
}