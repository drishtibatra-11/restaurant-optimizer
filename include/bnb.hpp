#pragma once
#include "graph.hpp"
#include "model.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <deque>
#include <queue>
#include <utility>
#include <vector>

// =====================================================================
// Exact seating decision: give every reservation its own table so that
// the total cost is as small as possible (an assignment problem).
//   cost[i][j] = cost of seating reservation i at table j, INF = not allowed.
//   There are n reservations and m >= n tables; each table is used at most once.
// =====================================================================
using CostMatrix = std::vector<std::vector<long long>>;

struct AssignResult {
    bool ok = false;                 // false: the matrix is not valid input
    bool feasible = false;           // false: no way to seat everyone
    long long cost = INF;            // total cost of the best assignment
    std::vector<int> assign;         // assign[i] = table chosen for reservation i
    long long generated = 0;         // nodes put on the frontier (branch and bound)
    long long expanded = 0;          // nodes whose children were created
    long long pruned = 0;            // nodes thrown away because their bound could not win
    long long peakFrontier = 0;      // most nodes waiting at the same time (memory)
    double ms = 0;                   // running time
};

enum class Strategy { FIFO, LIFO, LeastCost };

inline const char* strategyName(Strategy s) {
    return s == Strategy::FIFO ? "FIFO" : s == Strategy::LIFO ? "LIFO" : "LeastCost";
}

namespace detail {

// every row must have the same length, and at most maxCols columns
inline bool costsValid(const CostMatrix& c, std::size_t maxCols, std::size_t& m) {
    m = c.empty() ? 0 : c[0].size();
    if (m > maxCols) return false;
    for (const auto& row : c) if (row.size() != m) return false;
    return true;
}

// Lower bound for the rows fromRow..n-1: every row takes its cheapest table that is still
// free. Tables are shared in reality, so the true cost can only be higher.
// Returns INF if some row has no allowed table left.
inline long long lowerBound(const CostMatrix& c, int m, int fromRow, unsigned mask) {
    long long sum = 0;
    for (std::size_t r = static_cast<std::size_t>(fromRow); r < c.size(); ++r) {
        long long best = INF;
        for (int j = 0; j < m; ++j)
            if (!(mask >> j & 1) && c[r][j] < best) best = c[r][j];
        if (best >= INF) return INF;
        sum += best;
    }
    return sum;
}

inline double msSince(std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

struct Node {
    int level;                // how many reservations are already seated
    unsigned mask;            // tables already used
    long long cost;           // cost so far
    long long bound;          // cost so far + lower bound for the rest
    std::vector<int> assign;  // tables chosen so far
};

// For the least-cost queue: smallest bound first, deeper nodes first on ties
struct NodeWorse {
    bool operator()(const Node& a, const Node& b) const {
        if (a.bound != b.bound) return a.bound > b.bound;
        return a.level < b.level;
    }
};

// The waiting line of nodes. FIFO = queue, LIFO = stack, LeastCost = priority queue.
class Frontier {
public:
    explicit Frontier(Strategy s) : s_(s) {}
    void push(Node n) {
        if (s_ == Strategy::LeastCost) pq_.push(std::move(n));
        else dq_.push_back(std::move(n));
    }
    Node pop() {
        if (s_ == Strategy::LeastCost) { Node n = pq_.top(); pq_.pop(); return n; }
        if (s_ == Strategy::FIFO) { Node n = std::move(dq_.front()); dq_.pop_front(); return n; }
        Node n = std::move(dq_.back()); dq_.pop_back(); return n;
    }
    bool empty() const { return s_ == Strategy::LeastCost ? pq_.empty() : dq_.empty(); }
    std::size_t size() const { return s_ == Strategy::LeastCost ? pq_.size() : dq_.size(); }
private:
    Strategy s_;
    std::deque<Node> dq_;
    std::priority_queue<Node, std::vector<Node>, NodeWorse> pq_;
};

} // namespace detail

// Number of nodes in the full search tree (no pruning): sum over k of m!/(m-k)!
inline long long fullTreeNodes(int n, int m) {
    long long total = 1, level = 1;
    for (int k = 1; k <= n; ++k) {
        level *= (m - k + 1);
        total += level;
    }
    return total;
}

// Greedy: each reservation in turn takes its cheapest free table. Fast, not always optimal.
inline AssignResult assignGreedy(const CostMatrix& c) {
    AssignResult r;
    auto t0 = std::chrono::steady_clock::now();
    std::size_t mm;
    if (!detail::costsValid(c, 1u << 20, mm)) return r;
    r.ok = true;
    std::vector<bool> used(mm, false);
    long long total = 0;
    for (std::size_t i = 0; i < c.size(); ++i) {
        int pick = -1;
        for (std::size_t j = 0; j < mm; ++j)
            if (!used[j] && c[i][j] < INF && (pick < 0 || c[i][j] < c[i][pick])) pick = static_cast<int>(j);
        if (pick < 0) { r.assign.clear(); r.ms = detail::msSince(t0); return r; }
        used[pick] = true;
        r.assign.push_back(pick);
        total += c[i][pick];
    }
    r.feasible = true;
    r.cost = total;
    r.ms = detail::msSince(t0);
    return r;
}

// Branch and bound with the chosen strategy (FIFO, LIFO or least-cost).
// Each node is a partial assignment. A node is thrown away when its bound cannot beat
// the best complete assignment found so far. If greedyStart is true, the greedy answer is
// the first "best so far", so pruning begins immediately.
inline AssignResult branchAndBound(const CostMatrix& c, Strategy strat, bool greedyStart = false) {
    AssignResult r;
    auto t0 = std::chrono::steady_clock::now();
    std::size_t mm;
    if (!detail::costsValid(c, 30, mm)) return r;
    r.ok = true;
    int n = static_cast<int>(c.size()), m = static_cast<int>(mm);
    if (n == 0) { r.feasible = true; r.cost = 0; r.ms = detail::msSince(t0); return r; }
    if (n > m) { r.ms = detail::msSince(t0); return r; }

    long long best = INF;
    std::vector<int> bestAssign;
    if (greedyStart) {
        AssignResult g = assignGreedy(c);
        if (g.feasible) { best = g.cost; bestAssign = g.assign; }
    }
    detail::Node root{0, 0u, 0, detail::lowerBound(c, m, 0, 0u), {}};
    if (root.bound >= INF) { r.ms = detail::msSince(t0); return r; }

    detail::Frontier f(strat);
    f.push(root);
    r.generated = 1;
    r.peakFrontier = 1;
    while (!f.empty()) {
        detail::Node x = f.pop();
        if (x.bound >= best) {                       // cannot beat what we already have
            ++r.pruned;
            if (strat == Strategy::LeastCost) break; // everything left has a bound at least as big
            continue;
        }
        if (x.level == n) {                          // a complete assignment that is better
            best = x.cost;
            bestAssign = x.assign;
            continue;
        }
        ++r.expanded;
        for (int j = 0; j < m; ++j) {
            if (x.mask >> j & 1) continue;
            if (c[x.level][j] >= INF) continue;
            detail::Node ch;
            ch.level = x.level + 1;
            ch.mask = x.mask | (1u << j);
            ch.cost = x.cost + c[x.level][j];
            long long lb = detail::lowerBound(c, m, ch.level, ch.mask);
            if (lb >= INF || ch.cost + lb >= best) { ++r.pruned; continue; }
            ch.bound = ch.cost + lb;
            ch.assign = x.assign;
            ch.assign.push_back(j);
            f.push(std::move(ch));
            ++r.generated;
            r.peakFrontier = std::max<long long>(r.peakFrontier, static_cast<long long>(f.size()));
        }
    }
    if (best < INF) { r.feasible = true; r.cost = best; r.assign = bestAssign; }
    r.ms = detail::msSince(t0);
    return r;
}

// Dynamic programming alternative: dp[mask] = cheapest way to seat the first popcount(mask)
// reservations at exactly the tables in mask. Theta(2^m * m) time, Theta(2^m) memory.
inline AssignResult assignDP(const CostMatrix& c) {
    AssignResult r;
    auto t0 = std::chrono::steady_clock::now();
    std::size_t mm;
    if (!detail::costsValid(c, 22, mm)) return r;
    r.ok = true;
    std::size_t n = c.size();
    if (n == 0) { r.feasible = true; r.cost = 0; r.ms = detail::msSince(t0); return r; }
    if (n > mm) { r.ms = detail::msSince(t0); return r; }
    std::size_t full = static_cast<std::size_t>(1) << mm;
    std::vector<long long> dp(full, INF);
    std::vector<int> par(full, -1);
    dp[0] = 0;
    long long best = INF;
    std::size_t bestMask = 0;
    for (std::size_t mask = 0; mask < full; ++mask) {
        if (dp[mask] >= INF) continue;
        std::size_t k = 0;
        for (std::size_t x = mask; x; x &= x - 1) ++k;        // number of seated reservations
        if (k == n) {
            if (dp[mask] < best) { best = dp[mask]; bestMask = mask; }
            continue;
        }
        for (std::size_t j = 0; j < mm; ++j) {
            if (mask >> j & 1) continue;
            if (c[k][j] >= INF) continue;
            std::size_t nm = mask | (static_cast<std::size_t>(1) << j);
            if (dp[mask] + c[k][j] < dp[nm]) { dp[nm] = dp[mask] + c[k][j]; par[nm] = static_cast<int>(j); }
        }
    }
    if (best < INF) {
        r.feasible = true;
        r.cost = best;
        r.assign.assign(n, -1);
        std::size_t mask = bestMask;
        while (mask) {
            int j = par[mask];
            std::size_t k = 0;
            for (std::size_t x = mask; x; x &= x - 1) ++k;
            r.assign[k - 1] = j;
            mask &= ~(static_cast<std::size_t>(1) << j);
        }
    }
    r.ms = detail::msSince(t0);
    return r;
}

// ---------- Building the cost matrix from restaurant data ----------

// Walking distance from the kitchen to every table (same order as `tables`)
inline std::vector<long long> kitchenWalk(const std::vector<Table>& tables) {
    ShortestPaths sp = dijkstra(buildFloorPlan(tables), 0);
    std::vector<long long> walk(tables.size(), 0);
    for (std::size_t j = 0; j < tables.size(); ++j) walk[j] = sp.dist[j + 1];
    return walk;
}

// cost = 3 per empty seat + 10 if the table is outside the preferred zone + walking distance.
// INF if the party does not fit.
inline CostMatrix seatingCosts(const std::vector<Reservation>& res, const std::vector<Table>& tables,
                               const std::vector<long long>& walk) {
    CostMatrix c(res.size(), std::vector<long long>(tables.size(), INF));
    for (std::size_t i = 0; i < res.size(); ++i)
        for (std::size_t j = 0; j < tables.size(); ++j) {
            if (res[i].party > tables[j].seats) continue;
            long long cost = 3LL * (tables[j].seats - res[i].party) + walk[j];
            if (res[i].prefZone != 0 && res[i].prefZone != tables[j].zone) cost += 10;
            c[i][j] = cost;
        }
    return c;
}