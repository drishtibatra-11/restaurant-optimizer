#pragma once
#include "model.hpp"
#include <algorithm>
#include <climits>
#include <cstddef>
#include <functional>
#include <queue>
#include <utility>
#include <vector>

constexpr long long INF = LLONG_MAX / 4;   // "no path yet"

struct Edge { int to; long long w; };

// Weighted graph stored as adjacency lists.
class Graph {
public:
    explicit Graph(int n = 0) : adj_(n < 0 ? 0 : static_cast<std::size_t>(n)) {}
    int size() const { return static_cast<int>(adj_.size()); }

    // Returns false if u or v is not a node. Undirected by default.
    bool addEdge(int u, int v, long long w, bool undirected = true) {
        if (u < 0 || v < 0 || u >= size() || v >= size()) return false;
        adj_[u].push_back({v, w});
        if (undirected && u != v) adj_[v].push_back({u, w});
        if (w < 0) hasNegative_ = true;
        return true;
    }
    const std::vector<Edge>& neighbors(int u) const { return adj_[u]; }
    bool hasNegativeEdge() const { return hasNegative_; }

private:
    std::vector<std::vector<Edge>> adj_;
    bool hasNegative_ = false;
};

struct ShortestPaths {
    bool ok;                      // false: bad source, or a negative edge exists
    std::vector<long long> dist;  // INF if unreachable
    std::vector<int> parent;      // previous node on the shortest path, -1 for none
};

// Dijkstra with a min-heap: Theta((V + E) log V). Needs non-negative weights.
inline ShortestPaths dijkstra(const Graph& g, int src) {
    int n = g.size();
    ShortestPaths r{false, std::vector<long long>(n, INF), std::vector<int>(n, -1)};
    if (src < 0 || src >= n || g.hasNegativeEdge()) return r;
    r.ok = true;

    using Item = std::pair<long long, int>;   // (distance, node)
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> pq;
    r.dist[src] = 0;
    pq.push({0, src});
    while (!pq.empty()) {
        Item top = pq.top();
        pq.pop();
        int u = top.second;
        if (top.first > r.dist[u]) continue;       // outdated heap entry
        for (const Edge& e : g.neighbors(u)) {
            long long nd = top.first + e.w;
            if (nd < r.dist[e.to]) {
                r.dist[e.to] = nd;
                r.parent[e.to] = u;
                pq.push({nd, e.to});
            }
        }
    }
    return r;
}

// Nodes on the shortest path src -> target, in order. Empty if there is none.
inline std::vector<int> pathTo(const ShortestPaths& r, int src, int target) {
    std::vector<int> path;
    if (!r.ok || target < 0 || target >= static_cast<int>(r.dist.size())) return path;
    if (r.dist[target] >= INF) return path;
    for (int v = target; v != -1; v = r.parent[v]) path.push_back(v);
    std::reverse(path.begin(), path.end());
    if (path.front() != src) path.clear();
    return path;
}

// Floor plan: node 0 = kitchen, node i+1 = tables[i].
// Tables sit on a grid; neighbours are joined by walking distances of 2..6.
// The kitchen is joined to every table in the first row.
inline Graph buildFloorPlan(const std::vector<Table>& tables) {
    int n = static_cast<int>(tables.size());
    Graph g(n + 1);
    int cols = 1;
    while (cols * cols < n) ++cols;
    for (int i = 0; i < n; ++i) {
        int c = i % cols;
        if (c + 1 < cols && i + 1 < n) g.addEdge(i + 1, i + 2, 2 + (i * 7 + 3) % 5);
        if (i + cols < n)              g.addEdge(i + 1, i + cols + 1, 2 + (i * 3 + 1) % 5);
        if (i < cols)                  g.addEdge(0, i + 1, 5 + c);
    }
    return g;
}