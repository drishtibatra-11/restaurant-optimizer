#pragma once
#include "graph.hpp"
#include "sort.hpp"
#include <cstddef>
#include <functional>
#include <numeric>
#include <queue>
#include <utility>
#include <vector>

// ---------- Union-find (disjoint sets) ----------
// Path compression + union by size: almost constant time per operation.
class UnionFind {
public:
    explicit UnionFind(int n) : parent_(n), size_(n, 1), sets_(n) {
        std::iota(parent_.begin(), parent_.end(), 0);
    }
    int find(int x) {
        while (parent_[x] != x) {
            parent_[x] = parent_[parent_[x]];   // path halving
            x = parent_[x];
        }
        return x;
    }
    // Returns false if a and b were already in the same set.
    bool unite(int a, int b) {
        a = find(a);
        b = find(b);
        if (a == b) return false;
        if (size_[a] < size_[b]) std::swap(a, b);
        parent_[b] = a;
        size_[a] += size_[b];
        --sets_;
        return true;
    }
    int sets() const { return sets_; }

private:
    std::vector<int> parent_, size_;
    int sets_;
};

// ---------- Minimum spanning tree ----------
struct MSTEdge { int u, v; long long w; };

struct MSTResult {
    long long weight = 0;          // total weight of the tree (or forest)
    std::vector<MSTEdge> edges;    // the chosen edges
    bool connected = true;         // false if the graph has several components
};

// Every undirected edge once (u <= v). Needs a graph built with undirected edges.
inline std::vector<MSTEdge> edgeList(const Graph& g) {
    std::vector<MSTEdge> out;
    for (int u = 0; u < g.size(); ++u)
        for (const Edge& e : g.neighbors(u))
            if (u <= e.to) out.push_back({u, e.to, e.w});
    return out;
}

// Kruskal: sort the edges, add each one that joins two different components.
inline MSTResult kruskal(const Graph& g) {
    MSTResult r;
    int n = g.size();
    std::vector<MSTEdge> es = edgeList(g);
    mergeSort(es, [](const MSTEdge& a, const MSTEdge& b) { return a.w < b.w; });
    UnionFind uf(n);
    for (const MSTEdge& e : es)
        if (uf.unite(e.u, e.v)) {
            r.edges.push_back(e);
            r.weight += e.w;
        }
    r.connected = (n == 0) || uf.sets() == 1;
    return r;
}

// Prim: grow from node 0, then restart from every node not yet reached (forest).
inline MSTResult prim(const Graph& g) {
    MSTResult r;
    int n = g.size();
    std::vector<bool> inTree(n, false);
    int components = 0;
    using Item = std::pair<long long, std::pair<int, int>>;   // (weight, (from, to))
    for (int start = 0; start < n; ++start) {
        if (inTree[start]) continue;
        ++components;
        std::priority_queue<Item, std::vector<Item>, std::greater<Item>> pq;
        inTree[start] = true;
        for (const Edge& e : g.neighbors(start)) pq.push({e.w, {start, e.to}});
        while (!pq.empty()) {
            Item top = pq.top();
            pq.pop();
            int from = top.second.first, to = top.second.second;
            if (inTree[to]) continue;                  // would form a cycle
            inTree[to] = true;
            r.edges.push_back({from, to, top.first});
            r.weight += top.first;
            for (const Edge& e : g.neighbors(to))
                if (!inTree[e.to]) pq.push({e.w, {to, e.to}});
        }
    }
    r.connected = (n == 0) || components == 1;
    return r;
}