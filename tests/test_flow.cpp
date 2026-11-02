#include "csv_loader.hpp"
#include "flow.hpp"
#include "test.hpp"
#include <random>
#include <string>
#include <vector>

struct Edge3 { int u, v; long long cap; };

static FlowNetwork build(int n, const std::vector<Edge3>& es, std::vector<int>* ids = nullptr) {
    FlowNetwork net(n);
    for (const Edge3& e : es) {
        int id = net.addEdge(e.u, e.v, e.cap);
        if (ids) ids->push_back(id);
    }
    return net;
}

// Oracle (max-flow min-cut theorem): the maximum flow equals the cheapest cut.
// Try every set S that contains s and not t; the cut is the capacity of the edges leaving S.
static long long minCut(int n, const std::vector<Edge3>& es, int s, int t) {
    long long best = -1;
    for (unsigned mask = 0; mask < (1u << n); ++mask) {
        if (!(mask >> s & 1) || (mask >> t & 1)) continue;
        long long cut = 0;
        for (const Edge3& e : es)
            if ((mask >> e.u & 1) && !(mask >> e.v & 1)) cut += e.cap;
        if (best < 0 || cut < best) best = cut;
    }
    return best;
}

// The flow must respect capacities and be conserved at every node except s and t.
static bool flowValid(const FlowNetwork& net, int s, int t, long long total) {
    std::vector<long long> balance(net.size(), 0);
    for (int k = 0; k < net.edgeCount(); ++k) {
        int id = 2 * k;
        long long f = net.flowOn(id);
        if (f < 0 || f > net.capacity(id)) return false;
        balance[net.from(id)] -= f;
        balance[net.to(id)] += f;
    }
    for (int v = 0; v < net.size(); ++v) {
        if (v == s || v == t) continue;
        if (balance[v] != 0) return false;
    }
    return balance[t] == total && balance[s] == -total;
}

// Oracle for matching: try every pairing (small sizes only)
static void bruteMatch(const std::vector<std::vector<int>>& adj, std::size_t u,
                       std::vector<bool>& used, int cur, int& best) {
    if (u == adj.size()) { if (cur > best) best = cur; return; }
    bruteMatch(adj, u + 1, used, cur, best);                    // leave u unmatched
    for (int r : adj[u])
        if (!used[r]) { used[r] = true; bruteMatch(adj, u + 1, used, cur + 1, best); used[r] = false; }
}

static bool matchingValid(int nL, int nR, const std::vector<std::vector<int>>& adj, const MatchResult& m) {
    if (static_cast<int>(m.matchLeft.size()) != nL || static_cast<int>(m.matchRight.size()) != nR) return false;
    int count = 0;
    for (int l = 0; l < nL; ++l) {
        int r = m.matchLeft[l];
        if (r < 0) continue;
        ++count;
        bool edge = false;
        for (int x : adj[l]) if (x == r) edge = true;
        if (!edge || r >= nR || m.matchRight[r] != l) return false;
    }
    return count == m.size;
}

int main() {
    // bad input
    {
        FlowNetwork net(3);
        CHECK(net.addEdge(0, 5, 1) == -1 && net.addEdge(-1, 1, 1) == -1 && net.addEdge(0, 1, -2) == -1);
        net.addEdge(0, 1, 4);
        CHECK(net.maxFlowBFS(0, 0) == 0 && net.maxFlowDFS(0, 9) == 0 && net.maxFlowBFS(-1, 1) == 0);
        CHECK(net.maxFlowBFS(0, 2) == 0);                       // no path at all
    }

    // textbook network: maximum flow is 23
    {
        std::vector<Edge3> es = {{0, 1, 16}, {0, 2, 13}, {1, 2, 10}, {2, 1, 4}, {1, 3, 12},
                                 {3, 2, 9}, {2, 4, 14}, {4, 3, 7}, {3, 5, 20}, {4, 5, 4}};
        FlowNetwork a = build(6, es), b = build(6, es);
        CHECK(a.maxFlowBFS(0, 5) == 23 && flowValid(a, 0, 5, 23));
        CHECK(b.maxFlowDFS(0, 5) == 23 && flowValid(b, 0, 5, 23));
        CHECK(minCut(6, es, 0, 5) == 23);
        a.reset();                                              // reset lets us solve again
        CHECK(a.maxFlowDFS(0, 5) == 23 && flowValid(a, 0, 5, 23));
        CHECK(a.augmentations() > 0);
    }

    // parallel edges add up; a zero-capacity edge carries nothing; a self-loop is harmless
    {
        FlowNetwork net(3);
        net.addEdge(0, 1, 3);
        net.addEdge(0, 1, 4);
        net.addEdge(1, 1, 9);
        net.addEdge(1, 2, 10);
        int z = net.addEdge(1, 2, 0);
        CHECK(net.maxFlowBFS(0, 2) == 7 && net.flowOn(z) == 0);
    }

    // 300 random networks: both algorithms equal the min cut, and the flows are valid
    {
        std::mt19937 rng(141);
        bool valueOk = true, validOk = true;
        for (int trial = 0; trial < 300; ++trial) {
            int n = 2 + static_cast<int>(rng() % 7);
            std::vector<Edge3> es;
            int m = static_cast<int>(rng() % 20);
            for (int i = 0; i < m; ++i)
                es.push_back({static_cast<int>(rng() % n), static_cast<int>(rng() % n),
                              static_cast<long long>(rng() % 12)});
            int s = 0, t = n - 1;
            long long expect = minCut(n, es, s, t);
            FlowNetwork a = build(n, es), b = build(n, es);
            long long fa = a.maxFlowBFS(s, t), fb = b.maxFlowDFS(s, t);
            if (fa != expect || fb != expect) valueOk = false;
            if (!flowValid(a, s, t, fa) || !flowValid(b, s, t, fb)) validOk = false;
        }
        CHECK(valueOk);
        CHECK(validOk);
    }

    // matching: hand example. l0:{r0,r1}, l1:{r0}, l2:{r1,r2} -> 3 pairs
    {
        std::vector<std::vector<int>> adj = {{0, 1}, {0}, {1, 2}};
        MatchResult m = bipartiteMatching(3, 3, adj);
        CHECK(m.size == 3 && matchingValid(3, 3, adj, m));
        CHECK(matchingViaFlow(3, 3, adj) == 3);
        CHECK(bipartiteMatching(2, 2, {{}, {}}).size == 0);                  // no edges
        CHECK(bipartiteMatching(0, 4, {}).size == 0);                         // nobody on the left
        CHECK(bipartiteMatching(2, 2, {{5}, {0}}).matchLeft.empty());         // edge to a missing node
        CHECK(bipartiteMatching(3, 3, {{0}}).matchLeft.empty());              // wrong number of rows
    }

    // 300 random bipartite graphs: Kuhn = brute force = maximum flow
    {
        std::mt19937 rng(142);
        bool sizeOk = true, validOk = true, flowOk = true;
        for (int trial = 0; trial < 300; ++trial) {
            int nL = static_cast<int>(rng() % 7), nR = static_cast<int>(rng() % 7);
            std::vector<std::vector<int>> adj(nL);
            for (int l = 0; l < nL; ++l)
                for (int r = 0; r < nR; ++r)
                    if (rng() % 3 == 0) adj[l].push_back(r);
            std::vector<bool> used(nR, false);
            int best = 0;
            bruteMatch(adj, 0, used, 0, best);
            MatchResult m = bipartiteMatching(nL, nR, adj);
            if (m.size != best) sizeOk = false;
            if (!matchingValid(nL, nR, adj, m)) validOk = false;
            if (matchingViaFlow(nL, nR, adj) != best) flowOk = false;
        }
        CHECK(sizeOk);
        CHECK(validOk);
        CHECK(flowOk);
    }

    // restaurant use: pair parties with tables they fit at (one party per table)
    {
        Dataset d;
        std::string err;
        if (loadDataset("data/medium", d, err)) {
            std::vector<Reservation> rush;
            for (const Reservation& r : d.reservations)
                if (r.start >= 1080 && r.start < 1100) rush.push_back(r);
            int nL = static_cast<int>(rush.size()), nR = static_cast<int>(d.tables.size());
            std::vector<std::vector<int>> adj(nL);
            for (int i = 0; i < nL; ++i)
                for (int j = 0; j < nR; ++j)
                    if (rush[i].party <= d.tables[j].seats) adj[i].push_back(j);
            MatchResult m = bipartiteMatching(nL, nR, adj);
            CHECK(matchingValid(nL, nR, adj, m));
            CHECK(m.size == matchingViaFlow(nL, nR, adj));
            CHECK(m.size <= std::min(nL, nR));
            std::cout << "  rush matching: " << m.size << " of " << nL << " parties get a table of "
                      << nR << "\n";
        } else {
            CHECK(false);  // run "make data" first
        }
    }

    return TEST_SUMMARY();
}