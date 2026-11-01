#include "csv_loader.hpp"
#include "graph.hpp"
#include "test.hpp"
#include <random>
#include <string>
#include <vector>

// Independent oracle: keep relaxing every edge until nothing changes.
static std::vector<long long> oracle(const Graph& g, int src) {
    int n = g.size();
    std::vector<long long> dist(n, INF);
    dist[src] = 0;
    bool changed = true;
    for (int round = 0; round < n && changed; ++round) {
        changed = false;
        for (int u = 0; u < n; ++u) {
            if (dist[u] >= INF) continue;
            for (const Edge& e : g.neighbors(u))
                if (dist[u] + e.w < dist[e.to]) { dist[e.to] = dist[u] + e.w; changed = true; }
        }
    }
    return dist;
}

// cheapest edge u -> v, or -1 if there is none
static long long minEdge(const Graph& g, int u, int v) {
    long long best = -1;
    for (const Edge& e : g.neighbors(u))
        if (e.to == v && (best < 0 || e.w < best)) best = e.w;
    return best;
}

// The path must start at src, end at target, use real edges, and cost exactly dist[target].
static bool pathOk(const Graph& g, const ShortestPaths& r, int src, int target) {
    std::vector<int> p = pathTo(r, src, target);
    if (r.dist[target] >= INF) return p.empty();
    if (p.empty() || p.front() != src || p.back() != target) return false;
    long long sum = 0;
    for (std::size_t i = 1; i < p.size(); ++i) {
        long long w = minEdge(g, p[i - 1], p[i]);
        if (w < 0) return false;
        sum += w;
    }
    return sum == r.dist[target];
}

int main() {
    // empty graph: no valid source
    CHECK(!dijkstra(Graph(0), 0).ok);

    // hand example. Edges: 0-1 (4), 0-2 (1), 2-1 (2), 1-3 (1), 2-3 (5). Node 4 is isolated.
    {
        Graph g(5);
        g.addEdge(0, 1, 4);
        g.addEdge(0, 2, 1);
        g.addEdge(2, 1, 2);
        g.addEdge(1, 3, 1);
        g.addEdge(2, 3, 5);
        ShortestPaths r = dijkstra(g, 0);
        CHECK(r.ok);
        CHECK(r.dist[0] == 0 && r.dist[1] == 3 && r.dist[2] == 1 && r.dist[3] == 4);
        CHECK(r.dist[4] == INF);
        std::vector<int> p = pathTo(r, 0, 3);
        CHECK(p.size() == 4 && p[0] == 0 && p[1] == 2 && p[2] == 1 && p[3] == 3);
        CHECK(pathTo(r, 0, 4).empty());           // unreachable
        CHECK(pathTo(r, 0, 0).size() == 1);       // the source itself
        CHECK(pathTo(r, 0, 9).empty());           // not a node
    }

    // directed edges only go one way
    {
        Graph g(3);
        g.addEdge(0, 1, 5, false);
        g.addEdge(1, 2, 5, false);
        CHECK(dijkstra(g, 0).dist[2] == 10);
        CHECK(dijkstra(g, 2).dist[0] == INF);
    }

    // negative edge and invalid source are rejected
    {
        Graph g(2);
        g.addEdge(0, 1, -3);
        CHECK(!dijkstra(g, 0).ok);
        CHECK(!dijkstra(Graph(2), 5).ok);
        CHECK(!Graph(2).addEdge(0, 7, 1));       // edge to a node that does not exist
    }

    // 300 random graphs (directed and undirected, weights 0..20) against the oracle
    {
        std::mt19937 rng(51);
        bool distOk = true, pathsOk = true;
        for (int trial = 0; trial < 300; ++trial) {
            int n = 1 + static_cast<int>(rng() % 15);
            Graph g(n);
            int m = static_cast<int>(rng() % 40);
            bool undirected = trial % 2 == 0;
            for (int i = 0; i < m; ++i)
                g.addEdge(static_cast<int>(rng() % n), static_cast<int>(rng() % n),
                          static_cast<long long>(rng() % 21), undirected);
            int src = static_cast<int>(rng() % n);
            ShortestPaths r = dijkstra(g, src);
            if (!r.ok || r.dist != oracle(g, src)) distOk = false;
            for (int t = 0; t < n; ++t)
                if (!pathOk(g, r, src, t)) pathsOk = false;
        }
        CHECK(distOk);
        CHECK(pathsOk);
    }

    // restaurant use: shortest waiter path from the kitchen (node 0) to every table
    {
        Dataset d;
        std::string err;
        if (loadDataset("data/medium", d, err)) {
            Graph plan = buildFloorPlan(d.tables);
            CHECK(plan.size() == static_cast<int>(d.tables.size()) + 1);
            ShortestPaths r = dijkstra(plan, 0);
            CHECK(r.ok && r.dist == oracle(plan, 0));
            bool reach = true, paths = true;
            for (int t = 1; t < plan.size(); ++t) {
                if (r.dist[t] >= INF) reach = false;
                if (!pathOk(plan, r, 0, t)) paths = false;
            }
            CHECK(reach);
            CHECK(paths);
        } else {
            CHECK(false);  // run "make data" first
        }
    }

    // large floor plan (2000 tables): no edge can offer a shortcut that Dijkstra missed
    {
        Dataset d;
        std::string err;
        if (loadDataset("data/large", d, err)) {
            Graph plan = buildFloorPlan(d.tables);
            ShortestPaths r = dijkstra(plan, 0);
            bool ok = r.ok;
            for (int u = 0; u < plan.size() && ok; ++u) {
                if (r.dist[u] >= INF) { ok = false; break; }
                for (const Edge& e : plan.neighbors(u))
                    if (r.dist[e.to] > r.dist[u] + e.w) ok = false;
            }
            CHECK(ok);
        } else {
            CHECK(false);  // run "make data" first
        }
    }

    return TEST_SUMMARY();
}