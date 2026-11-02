#pragma once
#include <algorithm>
#include <climits>
#include <cstddef>
#include <queue>
#include <vector>

// =====================================================================
// Maximum flow (Ford-Fulkerson with DFS, and Edmonds-Karp = Ford-Fulkerson
// with BFS) and maximum bipartite matching.
// =====================================================================
class FlowNetwork {
public:
    explicit FlowNetwork(int n = 0) : adj_(static_cast<std::size_t>(n < 0 ? 0 : n)) {}
    int size() const { return static_cast<int>(adj_.size()); }

    // Adds an edge u -> v with a capacity. Returns the edge id, or -1 if the input is bad.
    int addEdge(int u, int v, long long cap) {
        if (u < 0 || v < 0 || u >= size() || v >= size() || cap < 0) return -1;
        int id = static_cast<int>(edges_.size());
        edges_.push_back({u, v, cap, cap});       // forward edge
        edges_.push_back({v, u, 0, 0});           // backward edge (the residual)
        adj_[u].push_back(id);
        adj_[v].push_back(id + 1);
        return id;
    }
    int edgeCount() const { return static_cast<int>(edges_.size() / 2); }
    int from(int id) const { return edges_[id].from; }
    int to(int id) const { return edges_[id].to; }
    long long capacity(int id) const { return edges_[id].orig; }
    long long flowOn(int id) const { return edges_[id].orig - edges_[id].res; }
    long long augmentations() const { return aug_; }

    // Undo all flow so the network can be solved again.
    void reset() {
        for (E& e : edges_) e.res = e.orig;
        aug_ = 0;
    }

    // Ford-Fulkerson: find any augmenting path with DFS. Time O(E * maxflow).
    long long maxFlowDFS(int s, int t) {
        aug_ = 0;
        if (!valid(s, t)) return 0;
        long long total = 0;
        std::vector<char> seen;
        while (true) {
            seen.assign(adj_.size(), 0);
            long long f = dfs(s, t, LLONG_MAX, seen);
            if (f == 0) break;
            total += f;
            ++aug_;
        }
        return total;
    }

    // Edmonds-Karp: always augment along a shortest path (BFS). Time O(V * E^2).
    long long maxFlowBFS(int s, int t) {
        aug_ = 0;
        if (!valid(s, t)) return 0;
        long long total = 0;
        std::vector<int> pe(adj_.size());
        while (true) {
            std::fill(pe.begin(), pe.end(), -1);
            std::queue<int> q;
            q.push(s);
            pe[s] = -2;
            while (!q.empty() && pe[t] == -1) {
                int u = q.front();
                q.pop();
                for (int id : adj_[u])
                    if (edges_[id].res > 0 && pe[edges_[id].to] == -1) {
                        pe[edges_[id].to] = id;
                        q.push(edges_[id].to);
                    }
            }
            if (pe[t] == -1) break;
            long long f = LLONG_MAX;
            for (int v = t; v != s; v = edges_[pe[v]].from) f = std::min(f, edges_[pe[v]].res);
            for (int v = t; v != s; v = edges_[pe[v]].from) {
                edges_[pe[v]].res -= f;
                edges_[pe[v] ^ 1].res += f;
            }
            total += f;
            ++aug_;
        }
        return total;
    }

private:
    struct E { int from, to; long long res, orig; };
    std::vector<E> edges_;
    std::vector<std::vector<int>> adj_;
    long long aug_ = 0;

    bool valid(int s, int t) const { return s >= 0 && t >= 0 && s < size() && t < size() && s != t; }

    long long dfs(int u, int t, long long f, std::vector<char>& seen) {
        if (u == t) return f;
        seen[u] = 1;
        for (int id : adj_[u]) {
            E& e = edges_[id];
            if (e.res > 0 && !seen[e.to]) {
                long long d = dfs(e.to, t, std::min(f, e.res), seen);
                if (d > 0) {
                    e.res -= d;
                    edges_[id ^ 1].res += d;
                    return d;
                }
            }
        }
        return 0;
    }
};

// ---------- Maximum bipartite matching (Kuhn: augmenting paths) ----------
// Left nodes 0..nL-1, right nodes 0..nR-1. adj[l] lists the right nodes l can be paired with.
// Time O(V * E).
struct MatchResult {
    int size = 0;
    std::vector<int> matchLeft;    // matchLeft[l] = right partner, or -1
    std::vector<int> matchRight;   // matchRight[r] = left partner, or -1
};

namespace detail {

inline bool kuhnTry(int u, const std::vector<std::vector<int>>& adj, std::vector<char>& seen,
                    std::vector<int>& mL, std::vector<int>& mR) {
    for (int r : adj[u]) {
        if (seen[r]) continue;
        seen[r] = 1;
        if (mR[r] < 0 || kuhnTry(mR[r], adj, seen, mL, mR)) {
            mL[u] = r;
            mR[r] = u;
            return true;
        }
    }
    return false;
}

} // namespace detail

inline MatchResult bipartiteMatching(int nL, int nR, const std::vector<std::vector<int>>& adj) {
    MatchResult res;
    if (nL < 0 || nR < 0 || static_cast<int>(adj.size()) != nL) return res;
    for (const auto& row : adj)
        for (int r : row) if (r < 0 || r >= nR) return res;          // bad edge: refuse
    res.matchLeft.assign(nL, -1);
    res.matchRight.assign(nR, -1);
    std::vector<char> seen;
    for (int u = 0; u < nL; ++u) {
        seen.assign(nR, 0);
        if (detail::kuhnTry(u, adj, seen, res.matchLeft, res.matchRight)) ++res.size;
    }
    return res;
}

// Same answer through maximum flow on a unit-capacity network (source -> left -> right -> sink).
inline int matchingViaFlow(int nL, int nR, const std::vector<std::vector<int>>& adj) {
    if (nL < 0 || nR < 0 || static_cast<int>(adj.size()) != nL) return 0;
    FlowNetwork net(nL + nR + 2);
    int s = nL + nR, t = s + 1;
    for (int l = 0; l < nL; ++l) {
        net.addEdge(s, l, 1);
        for (int r : adj[l]) net.addEdge(l, nL + r, 1);
    }
    for (int r = 0; r < nR; ++r) net.addEdge(nL + r, t, 1);
    return static_cast<int>(net.maxFlowBFS(s, t));
}