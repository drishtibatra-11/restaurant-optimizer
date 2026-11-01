#include "csv_loader.hpp"
#include "mst.hpp"
#include "test.hpp"
#include <random>
#include <string>
#include <vector>

// Oracle: try every subset of edges, keep the cheapest one that connects each component.
static long long bruteMST(const Graph& g) {
    int n = g.size();
    std::vector<MSTEdge> es = edgeList(g);
    int m = static_cast<int>(es.size());
    UnionFind all(n);
    for (const MSTEdge& e : es) all.unite(e.u, e.v);
    int need = all.sets();                       // components that the tree must keep
    long long best = -1;
    for (unsigned mask = 0; mask < (1u << m); ++mask) {
        UnionFind uf(n);
        long long w = 0;
        bool acyclic = true;
        for (int i = 0; i < m && acyclic; ++i)
            if (mask >> i & 1) {
                if (!uf.unite(es[i].u, es[i].v)) acyclic = false;
                w += es[i].w;
            }
        if (acyclic && uf.sets() == need && (best < 0 || w < best)) best = w;
    }
    return best;
}

static bool isForest(const Graph& g, const MSTResult& r) {
    UnionFind uf(g.size());
    long long sum = 0;
    for (const MSTEdge& e : r.edges) {
        if (!uf.unite(e.u, e.v)) return false;   // a cycle
        sum += e.w;
    }
    UnionFind all(g.size());
    for (const MSTEdge& e : edgeList(g)) all.unite(e.u, e.v);
    return sum == r.weight && uf.sets() == all.sets();   // spans every component
}

int main() {
    // union-find
    {
        UnionFind uf(6);
        CHECK(uf.sets() == 6);
        CHECK(uf.unite(0, 1) && uf.unite(2, 3) && uf.unite(1, 3));
        CHECK(!uf.unite(0, 2));                  // already connected
        CHECK(uf.find(0) == uf.find(3) && uf.find(0) != uf.find(4));
        CHECK(uf.sets() == 3);
    }

    // empty and single-node graphs
    {
        CHECK(kruskal(Graph(0)).weight == 0 && kruskal(Graph(0)).connected);
        CHECK(prim(Graph(0)).weight == 0 && prim(Graph(0)).connected);
        CHECK(kruskal(Graph(1)).edges.empty() && prim(Graph(1)).connected);
    }

    // hand example (weights worked out on paper): MST weight 16
    {
        Graph g(5);
        g.addEdge(0, 1, 2);
        g.addEdge(0, 3, 6);
        g.addEdge(1, 2, 3);
        g.addEdge(1, 3, 8);
        g.addEdge(1, 4, 5);
        g.addEdge(2, 4, 7);
        g.addEdge(3, 4, 9);
        MSTResult k = kruskal(g), p = prim(g);
        CHECK(k.weight == 16 && p.weight == 16);
        CHECK(k.edges.size() == 4 && p.edges.size() == 4);
        CHECK(k.connected && p.connected);
    }

    // disconnected graph: a spanning forest, flagged as not connected
    {
        Graph g(5);
        g.addEdge(0, 1, 4);
        g.addEdge(2, 3, 1);                      // node 4 is alone
        MSTResult k = kruskal(g), p = prim(g);
        CHECK(!k.connected && !p.connected);
        CHECK(k.weight == 5 && p.weight == 5);
        CHECK(k.edges.size() == 2 && p.edges.size() == 2);
    }

    // self-loops and parallel edges
    {
        Graph g(3);
        g.addEdge(0, 0, 1);                      // self-loop: never in a tree
        g.addEdge(0, 1, 9);
        g.addEdge(0, 1, 4);                      // cheaper parallel edge
        g.addEdge(1, 2, 6);
        CHECK(kruskal(g).weight == 10 && prim(g).weight == 10);
    }

    // all weights equal: any spanning tree works, weight = (n-1) * w
    {
        Graph g(6);
        for (int i = 0; i < 6; ++i)
            for (int j = i + 1; j < 6; ++j) g.addEdge(i, j, 3);
        CHECK(kruskal(g).weight == 15 && prim(g).weight == 15);
    }

    // 300 random graphs against the brute force
    {
        std::mt19937 rng(61);
        bool weightOk = true, structOk = true;
        for (int trial = 0; trial < 300; ++trial) {
            int n = 1 + static_cast<int>(rng() % 7);
            Graph g(n);
            int m = static_cast<int>(rng() % 12);
            for (int i = 0; i < m; ++i)
                g.addEdge(static_cast<int>(rng() % n), static_cast<int>(rng() % n),
                          static_cast<long long>(rng() % 15));
            if (edgeList(g).size() > 14) continue;       // keep the brute force fast
            MSTResult k = kruskal(g), p = prim(g);
            long long expect = bruteMST(g);
            if (k.weight != expect || p.weight != expect) weightOk = false;
            if (!isForest(g, k) || !isForest(g, p)) structOk = false;
        }
        CHECK(weightOk);
        CHECK(structOk);
    }

    // restaurant use: cheapest wiring for the floor plan
    {
        Dataset d;
        std::string err;
        if (loadDataset("data/medium", d, err)) {
            Graph plan = buildFloorPlan(d.tables);
            MSTResult k = kruskal(plan), p = prim(plan);
            CHECK(k.connected && p.connected);
            CHECK(k.weight == p.weight);
            CHECK(k.edges.size() == d.tables.size());    // (tables + 1 kitchen) - 1 edges
            CHECK(isForest(plan, k) && isForest(plan, p));
        } else {
            CHECK(false);  // run "make data" first
        }
    }

    // large floor plan: both algorithms agree
    {
        Dataset d;
        std::string err;
        if (loadDataset("data/large", d, err)) {
            Graph plan = buildFloorPlan(d.tables);
            MSTResult k = kruskal(plan), p = prim(plan);
            CHECK(k.connected && p.connected && k.weight == p.weight);
            CHECK(k.edges.size() == d.tables.size());
        } else {
            CHECK(false);  // run "make data" first
        }
    }

    return TEST_SUMMARY();
}