#pragma once
#include "model.hpp"
#include "search.hpp"
#include "sort.hpp"
#include <algorithm>
#include <cstddef>
#include <unordered_map>
#include <vector>

// Index over reservations: sorted by start time, bucketed into 15-minute slots,
// and a hash map for lookup by id. Built once, then answers queries quickly.
class ReservationIndex {
public:
    static constexpr int SLOT_MIN  = 15;   // minutes per slot
    static constexpr int NUM_SLOTS = 96;   // 24 hours * 4

    // Build: O(n log n) because of the merge sort
    explicit ReservationIndex(const std::vector<Reservation>& input)
        : sorted_(input), slots_(NUM_SLOTS) {
        mergeSort(sorted_, [](const Reservation& a, const Reservation& b) {
            return a.start < b.start;
        });
        starts_.reserve(sorted_.size());
        for (std::size_t i = 0; i < sorted_.size(); ++i) {
            const Reservation& r = sorted_[i];
            starts_.push_back(r.start);
            maxDur_ = std::max(maxDur_, r.dur);
            byId_[r.id] = i;
            slots_[slotOf(r.start)].push_back(i);
        }
    }

    std::size_t size() const { return sorted_.size(); }

    // Reservations with from <= start < to, in start order. O(log n + k)
    std::vector<Reservation> startingIn(int from, int to) const {
        std::vector<Reservation> out;
        if (from >= to) return out;
        std::size_t i = lowerBound(starts_, from), j = lowerBound(starts_, to);
        out.assign(sorted_.begin() + i, sorted_.begin() + j);
        return out;
    }

    // How many start in [from, to). O(log n)
    std::size_t countStartingIn(int from, int to) const {
        if (from >= to) return 0;
        return lowerBound(starts_, to) - lowerBound(starts_, from);
    }

    // Reservations in progress at time t: start <= t < end.
    // A reservation that started at or before t - maxDur has already ended,
    // so only starts in (t - maxDur, t] have to be looked at.
    std::vector<Reservation> activeAt(int t) const {
        std::vector<Reservation> out;
        if (sorted_.empty()) return out;
        std::size_t i = lowerBound(starts_, t - maxDur_ + 1);
        std::size_t j = lowerBound(starts_, t + 1);
        for (std::size_t k = i; k < j; ++k)
            if (sorted_[k].end() > t) out.push_back(sorted_[k]);
        return out;
    }

    // Reservations whose start falls in slot s (0..95). O(k)
    std::vector<Reservation> slotReservations(int s) const {
        std::vector<Reservation> out;
        if (s < 0 || s >= NUM_SLOTS) return out;
        for (std::size_t i : slots_[s]) out.push_back(sorted_[i]);
        return out;
    }

    std::size_t slotCount(int s) const {
        return (s < 0 || s >= NUM_SLOTS) ? 0 : slots_[s].size();
    }

    // Lookup by id, or nullptr if unknown. O(1) average.
    const Reservation* findById(int id) const {
        auto it = byId_.find(id);
        return it == byId_.end() ? nullptr : &sorted_[it->second];
    }

private:
    static int slotOf(int start) {
        return std::max(0, std::min(NUM_SLOTS - 1, start / SLOT_MIN));
    }

    std::vector<Reservation> sorted_;                 // sorted by start (stable)
    std::vector<std::vector<std::size_t>> slots_;     // slot -> indices into sorted_
    std::vector<int> starts_;                         // start times, for binary search
    std::unordered_map<int, std::size_t> byId_;       // id -> index into sorted_
    int maxDur_ = 0;                                  // longest reservation
};