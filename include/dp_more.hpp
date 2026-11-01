#pragma once
#include "model.hpp"
#include <algorithm>
#include <cstddef>
#include <vector>

// =====================================================================
// Week 4 parts B to E
// =====================================================================
#include "graph.hpp"
#include <climits>
#include <string>
#include <utility>

// ---------- Matrix chain multiplication ----------
struct ChainResult {
    long long cost = 0;     // fewest scalar multiplications
    std::string order;      // best bracketing, e.g. "((A1A2)A3)"
};

namespace detail {
inline std::string chainOrder(const std::vector<std::vector<int>>& s, int i, int j) {
    if (i == j) return "A" + std::to_string(i);
    int k = s[i][j];
    return "(" + chainOrder(s, i, k) + chainOrder(s, k + 1, j) + ")";
}
} // namespace detail

// p has n+1 numbers: matrix Ai is p[i-1] x p[i].
// Theta(n^3) time, Theta(n^2) memory.
inline ChainResult matrixChain(const std::vector<long long>& p) {
    ChainResult res;
    if (p.size() < 2) return res;
    int n = static_cast<int>(p.size()) - 1;
    std::vector<std::vector<long long>> m(n + 1, std::vector<long long>(n + 1, 0));
    std::vector<std::vector<int>> s(n + 1, std::vector<int>(n + 1, 0));
    for (int len = 2; len <= n; ++len)
        for (int i = 1; i + len - 1 <= n; ++i) {
            int j = i + len - 1;
            m[i][j] = LLONG_MAX;
            for (int k = i; k < j; ++k) {
                long long c = m[i][k] + m[k + 1][j] + p[i - 1] * p[k] * p[j];
                if (c < m[i][j]) { m[i][j] = c; s[i][j] = k; }
            }
        }
    res.cost = m[1][n];
    res.order = detail::chainOrder(s, 1, n);
    return res;
}

// Cost of simply multiplying from left to right: ((A1A2)A3)...  (for comparison)
inline long long chainCostLeftToRight(const std::vector<long long>& p) {
    long long c = 0;
    for (std::size_t k = 2; k < p.size(); ++k) c += p[0] * p[k - 1] * p[k];
    return c;
}

// ---------- Longest common subsequence (LCS) ----------
// Full table, Theta(n*m) time and memory; rebuilds one longest subsequence.
inline std::vector<int> lcs(const std::vector<int>& a, const std::vector<int>& b) {
    std::size_t n = a.size(), m = b.size();
    std::vector<std::vector<int>> L(n + 1, std::vector<int>(m + 1, 0));
    for (std::size_t i = 1; i <= n; ++i)
        for (std::size_t j = 1; j <= m; ++j)
            L[i][j] = (a[i - 1] == b[j - 1]) ? L[i - 1][j - 1] + 1
                                             : std::max(L[i - 1][j], L[i][j - 1]);
    std::vector<int> out;
    std::size_t i = n, j = m;
    while (i > 0 && j > 0) {
        if (a[i - 1] == b[j - 1]) { out.push_back(a[i - 1]); --i; --j; }
        else if (L[i - 1][j] >= L[i][j - 1]) --i;
        else --j;
    }
    std::reverse(out.begin(), out.end());
    return out;
}

// Length only, two rows: Theta(n*m) time, Theta(m) memory.
inline std::size_t lcsLength(const std::vector<int>& a, const std::vector<int>& b) {
    std::vector<int> prev(b.size() + 1, 0), cur(b.size() + 1, 0);
    for (std::size_t i = 1; i <= a.size(); ++i) {
        for (std::size_t j = 1; j <= b.size(); ++j)
            cur[j] = (a[i - 1] == b[j - 1]) ? prev[j - 1] + 1 : std::max(prev[j], cur[j - 1]);
        std::swap(prev, cur);
    }
    return static_cast<std::size_t>(prev[b.size()]);
}

// ---------- Optimal binary search tree ----------
// Keys are in sorted order; freq[i] is how often key i is looked up.
// Cost = sum of freq[i] * depth(i), where the root has depth 1.
struct OBSTResult {
    long long cost = 0;
    std::vector<int> depth;   // depth[i] = depth of key i in the best tree
};

// Theta(n^3) time, Theta(n^2) memory.
inline OBSTResult optimalBST(const std::vector<long long>& f) {
    OBSTResult res;
    int n = static_cast<int>(f.size());
    res.depth.assign(f.size(), 0);
    if (n == 0) return res;
    std::vector<long long> pre(n + 1, 0);
    for (int i = 0; i < n; ++i) pre[i + 1] = pre[i] + f[i];
    // cost[i][j] = best cost for the keys i..j-1
    std::vector<std::vector<long long>> cost(n + 1, std::vector<long long>(n + 1, 0));
    std::vector<std::vector<int>> root(n + 1, std::vector<int>(n + 1, -1));
    for (int len = 1; len <= n; ++len)
        for (int i = 0; i + len <= n; ++i) {
            int j = i + len;
            long long best = LLONG_MAX;
            for (int r = i; r < j; ++r) {
                long long c = cost[i][r] + cost[r + 1][j];
                if (c < best) { best = c; root[i][j] = r; }
            }
            cost[i][j] = best + pre[j] - pre[i];
        }
    res.cost = cost[0][n];
    struct Range { int i, j, d; };
    std::vector<Range> st;
    st.push_back({0, n, 1});
    while (!st.empty()) {
        Range x = st.back();
        st.pop_back();
        if (x.i >= x.j) continue;
        int r = root[x.i][x.j];
        res.depth[r] = x.d;
        st.push_back({x.i, r, x.d + 1});
        st.push_back({r + 1, x.j, x.d + 1});
    }
    return res;
}


// ---------- Bellman-Ford: shortest paths that may use negative weights ----------
struct BellmanFordResult {
    ShortestPaths sp;         // sp.ok is false only for a bad source
    bool negCycle = false;    // a negative cycle is reachable from the source
};

// Theta(V * E). Use pathTo(r.sp, ...) only when negCycle is false.
// With a negative cycle the distances are not meaningful.
inline BellmanFordResult bellmanFord(const Graph& g, int src) {
    int n = g.size();
    BellmanFordResult r;
    r.sp = ShortestPaths{false, std::vector<long long>(n, INF), std::vector<int>(n, -1)};
    if (src < 0 || src >= n) return r;
    r.sp.ok = true;
    r.sp.dist[src] = 0;
    for (int round = 0; round < n - 1; ++round) {
        bool changed = false;
        for (int u = 0; u < n; ++u) {
            if (r.sp.dist[u] >= INF) continue;
            for (const Edge& e : g.neighbors(u))
                if (r.sp.dist[u] + e.w < r.sp.dist[e.to]) {
                    r.sp.dist[e.to] = r.sp.dist[u] + e.w;
                    r.sp.parent[e.to] = u;
                    changed = true;
                }
        }
        if (!changed) break;
    }
    // one more round: if anything still improves, a negative cycle is reachable
    for (int u = 0; u < n && !r.negCycle; ++u) {
        if (r.sp.dist[u] >= INF) continue;
        for (const Edge& e : g.neighbors(u))
            if (r.sp.dist[u] + e.w < r.sp.dist[e.to]) { r.negCycle = true; break; }
    }
    return r;
}

// ---------- Floyd-Warshall: all pairs ----------
struct FWResult {
    std::vector<std::vector<long long>> dist;   // INF if there is no path
    std::vector<std::vector<int>> next;         // next node on the path u -> v, -1 if none
    bool negCycle = false;                      // some node has a negative cycle through it
};

// Theta(V^3) time, Theta(V^2) memory.
inline FWResult floydWarshall(const Graph& g) {
    int n = g.size();
    FWResult r;
    r.dist.assign(n, std::vector<long long>(n, INF));
    r.next.assign(n, std::vector<int>(n, -1));
    for (int i = 0; i < n; ++i) { r.dist[i][i] = 0; r.next[i][i] = i; }
    for (int u = 0; u < n; ++u)
        for (const Edge& e : g.neighbors(u))
            if (e.w < r.dist[u][e.to]) { r.dist[u][e.to] = e.w; r.next[u][e.to] = e.to; }
    for (int k = 0; k < n; ++k)
        for (int i = 0; i < n; ++i) {
            if (r.dist[i][k] >= INF) continue;
            for (int j = 0; j < n; ++j) {
                if (r.dist[k][j] >= INF) continue;
                long long nd = r.dist[i][k] + r.dist[k][j];
                if (nd < -INF) nd = -INF;               // stop numbers from overflowing
                if (nd < r.dist[i][j]) { r.dist[i][j] = nd; r.next[i][j] = r.next[i][k]; }
            }
        }
    for (int i = 0; i < n; ++i) if (r.dist[i][i] < 0) r.negCycle = true;
    return r;
}

// Nodes on the shortest path u -> v. Empty if there is none or a negative cycle exists.
inline std::vector<int> fwPath(const FWResult& r, int u, int v) {
    std::vector<int> p;
    int n = static_cast<int>(r.dist.size());
    if (r.negCycle || u < 0 || v < 0 || u >= n || v >= n || r.dist[u][v] >= INF) return p;
    p.push_back(u);
    int guard = 0;
    while (u != v) {
        u = r.next[u][v];
        p.push_back(u);
        if (++guard > n) { p.clear(); break; }
    }
    return p;
}

// ---------- Bounded travelling salesman (bitmask DP, n <= 15) ----------
using DistMatrix = std::vector<std::vector<long long>>;

struct TSPResult {
    bool ok = false;               // false: bad input (n = 0, n > 15) or no tour exists
    long long cost = INF;          // length of the round trip, including the way back
    std::vector<int> tour;         // visiting order, starting at node 0
};

// Held-Karp: Theta(2^n * n^2) time, Theta(2^n * n) memory. Node 0 is the start (kitchen).
// d[i][j] = INF means there is no direct way from i to j.
inline TSPResult tspDP(const DistMatrix& d) {
    TSPResult r;
    int n = static_cast<int>(d.size());
    if (n == 0 || n > 15) return r;
    for (const auto& row : d) if (static_cast<int>(row.size()) != n) return r;
    if (n == 1) { r.ok = true; r.cost = 0; r.tour = {0}; return r; }

    int m = n - 1;                                   // nodes 1..n-1 are bits 0..m-1
    std::size_t full = static_cast<std::size_t>(1) << m;
    std::vector<long long> dp(full * m, INF);
    std::vector<int> par(full * m, -1);
    for (int j = 0; j < m; ++j)
        if (d[0][j + 1] < INF) dp[(static_cast<std::size_t>(1) << j) * m + j] = d[0][j + 1];
    for (std::size_t mask = 1; mask < full; ++mask)
        for (int last = 0; last < m; ++last) {
            if (!(mask >> last & 1)) continue;
            long long cur = dp[mask * m + last];
            if (cur >= INF) continue;
            for (int nx = 0; nx < m; ++nx) {
                if (mask >> nx & 1) continue;
                long long w = d[last + 1][nx + 1];
                if (w >= INF) continue;
                std::size_t nm = mask | (static_cast<std::size_t>(1) << nx);
                if (cur + w < dp[nm * m + nx]) { dp[nm * m + nx] = cur + w; par[nm * m + nx] = last; }
            }
        }
    long long best = INF;
    int bestLast = -1;
    for (int last = 0; last < m; ++last) {
        long long c = dp[(full - 1) * m + last];
        if (c >= INF || d[last + 1][0] >= INF) continue;
        if (c + d[last + 1][0] < best) { best = c + d[last + 1][0]; bestLast = last; }
    }
    if (bestLast < 0) return r;
    r.ok = true;
    r.cost = best;
    std::vector<int> rev;
    std::size_t mask = full - 1;
    int last = bestLast;
    while (last >= 0) {
        rev.push_back(last + 1);
        int p = par[mask * m + last];
        mask &= ~(static_cast<std::size_t>(1) << last);
        last = p;
    }
    r.tour.push_back(0);
    for (std::size_t i = rev.size(); i-- > 0;) r.tour.push_back(rev[i]);
    return r;
}

// Nearest-neighbour heuristic: Theta(n^2), fast but not optimal (for comparison).
inline TSPResult tspNearest(const DistMatrix& d) {
    TSPResult r;
    int n = static_cast<int>(d.size());
    if (n == 0) return r;
    for (const auto& row : d) if (static_cast<int>(row.size()) != n) return r;
    std::vector<bool> seen(n, false);
    int cur = 0;
    seen[0] = true;
    r.tour.push_back(0);
    long long total = 0;
    for (int step = 1; step < n; ++step) {
        int pick = -1;
        for (int j = 0; j < n; ++j)
            if (!seen[j] && d[cur][j] < INF && (pick < 0 || d[cur][j] < d[cur][pick])) pick = j;
        if (pick < 0) { r.tour.clear(); return r; }
        total += d[cur][pick];
        seen[pick] = true;
        r.tour.push_back(pick);
        cur = pick;
    }
    if (n > 1) {
        if (d[cur][0] >= INF) { r.tour.clear(); return r; }
        total += d[cur][0];
    }
    r.ok = true;
    r.cost = total;
    return r;
}