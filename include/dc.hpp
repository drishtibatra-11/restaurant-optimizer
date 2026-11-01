#pragma once
#include <cstddef>
#include <vector>

// ---------- Maximum subarray ----------
struct SubResult {
    long long sum;
    long lo;   // first index of the best stretch
    long hi;   // last index of the best stretch (inclusive)
};

namespace detail {

inline SubResult maxSubRec(const std::vector<int>& a, long lo, long hi) {
    // inclusive range [lo, hi]
    if (lo == hi) return {a[lo], lo, lo};
    long mid = lo + (hi - lo) / 2;
    SubResult left  = maxSubRec(a, lo, mid);
    SubResult right = maxSubRec(a, mid + 1, hi);

    // best stretch that crosses the middle: extend left from mid, right from mid+1
    long long sum = 0, bestL = a[mid];
    long li = mid;
    for (long i = mid; i >= lo; --i) {
        sum += a[i];
        if (sum > bestL || i == mid) { bestL = sum; li = i; }
    }
    sum = 0;
    long long bestR = a[mid + 1];
    long ri = mid + 1;
    for (long j = mid + 1; j <= hi; ++j) {
        sum += a[j];
        if (sum > bestR || j == mid + 1) { bestR = sum; ri = j; }
    }
    SubResult cross = {bestL + bestR, li, ri};

    if (left.sum >= right.sum && left.sum >= cross.sum) return left;
    if (right.sum >= cross.sum) return right;
    return cross;
}

} // namespace detail

// Divide and conquer, Theta(n log n). Requires a non-empty vector.
inline SubResult maxSubarrayDC(const std::vector<int>& a) {
    return detail::maxSubRec(a, 0, static_cast<long>(a.size()) - 1);
}

// Kadane, O(n). Requires a non-empty vector.
inline SubResult maxSubarrayKadane(const std::vector<int>& a) {
    long long best = a[0], cur = a[0];
    long bestLo = 0, bestHi = 0, curLo = 0;
    for (long i = 1; i < static_cast<long>(a.size()); ++i) {
        if (cur < 0) { cur = a[i]; curLo = i; }
        else         { cur += a[i]; }
        if (cur > best) { best = cur; bestLo = curLo; bestHi = i; }
    }
    return {best, bestLo, bestHi};
}

// Brute force O(n^2), used only by tests as the oracle.
inline long long maxSubarrayBrute(const std::vector<int>& a) {
    long long best = a[0];
    for (std::size_t i = 0; i < a.size(); ++i) {
        long long s = 0;
        for (std::size_t j = i; j < a.size(); ++j) {
            s += a[j];
            if (s > best) best = s;
        }
    }
    return best;
}

// ---------- Fast power ----------
#include "model.hpp"

// Naive: e multiplications, Theta(e). Used as the checker.
inline unsigned long long powerNaive(unsigned long long b, unsigned e) {
    unsigned long long r = 1;
    for (unsigned i = 0; i < e; ++i) r *= b;
    return r;
}

// Divide and conquer: T(e) = T(e/2) + Theta(1) = Theta(log e).
// The result must fit in 64 bits (no overflow protection).
inline unsigned long long powerFast(unsigned long long b, unsigned e) {
    if (e == 0) return 1;
    unsigned long long half = powerFast(b, e / 2);
    return (e % 2 == 0) ? half * half : half * half * b;
}

// Same idea as a loop (look at the bits of e from the lowest one up).
inline unsigned long long powerIter(unsigned long long b, unsigned e) {
    unsigned long long r = 1;
    while (e > 0) {
        if (e & 1u) r *= b;
        b *= b;
        e >>= 1;
    }
    return r;
}

// (b ^ e) mod m, Theta(log e). A 128-bit intermediate avoids overflow.
inline unsigned long long powMod(unsigned long long b, unsigned long long e,
                                 unsigned long long m) {
    if (m == 1) return 0;
    unsigned long long r = 1;
    b %= m;
    while (e > 0) {
        if (e & 1ULL)
            r = static_cast<unsigned long long>(
                    static_cast<unsigned __int128>(r) * b % m);
        b = static_cast<unsigned long long>(
                static_cast<unsigned __int128>(b) * b % m);
        e >>= 1;
    }
    return r;
}

// Checksum code for a reservation, 0..1000000006.
inline unsigned long long reservationCode(const Reservation& r) {
    unsigned long long x = r.id * 131ULL + r.party * 7ULL + r.start;
    return powMod(x, 65537ULL, 1000000007ULL);
}

// ---------- Celebrity problem ----------
// knows[a][b] == true means person a knows person b.
// A celebrity is known by everyone else and knows nobody. Returns -1 if none.
using KnowsMatrix = std::vector<std::vector<bool>>;

struct CelebResult {
    int person;          // index of the celebrity, or -1
    long long calls;     // how many times knows[][] was read
};

namespace detail {

// true if c is known by everyone else and knows nobody (counts every read)
inline bool isCelebrity(const KnowsMatrix& k, int c, long long& calls) {
    int n = static_cast<int>(k.size());
    for (int i = 0; i < n; ++i) {
        if (i == c) continue;
        calls += 2;
        if (k[c][i] || !k[i][c]) return false;
    }
    return true;
}

// Divide and conquer on [lo, hi): returns the only possible candidate.
// T(n) = 2T(n/2) + Theta(1) = Theta(n)
inline int celebCandidate(const KnowsMatrix& k, int lo, int hi, long long& calls) {
    if (hi - lo == 1) return lo;
    int mid = lo + (hi - lo) / 2;
    int a = celebCandidate(k, lo, mid, calls);
    int b = celebCandidate(k, mid, hi, calls);
    ++calls;
    return k[a][b] ? b : a;      // if a knows b, a is out; otherwise b is out
}

} // namespace detail

// Brute force, Theta(n^2): test everyone.
inline CelebResult celebrityBrute(const KnowsMatrix& k) {
    long long calls = 0;
    int n = static_cast<int>(k.size());
    for (int c = 0; c < n; ++c)
        if (detail::isCelebrity(k, c, calls)) return {c, calls};
    return {-1, calls};
}

// Elimination, at most 3(n-1) reads.
inline CelebResult celebrityElim(const KnowsMatrix& k) {
    long long calls = 0;
    int n = static_cast<int>(k.size());
    if (n == 0) return {-1, 0};
    int cand = 0;
    for (int i = 1; i < n; ++i) {
        ++calls;
        if (k[cand][i]) cand = i;    // cand knows i, so cand cannot be the celebrity
    }
    return {detail::isCelebrity(k, cand, calls) ? cand : -1, calls};
}

// Divide and conquer candidate search, then one verification pass.
inline CelebResult celebrityDC(const KnowsMatrix& k) {
    long long calls = 0;
    int n = static_cast<int>(k.size());
    if (n == 0) return {-1, 0};
    int cand = detail::celebCandidate(k, 0, n, calls);
    return {detail::isCelebrity(k, cand, calls) ? cand : -1, calls};
}

// ---------- Matrix multiplication: classical vs Strassen ----------
#include <algorithm>

struct Matrix {
    int rows, cols;
    std::vector<long long> a;
    Matrix(int r = 0, int c = 0)
        : rows(r), cols(c), a(static_cast<std::size_t>(r) * c, 0) {}
    long long& at(int i, int j) { return a[static_cast<std::size_t>(i) * cols + j]; }
    long long at(int i, int j) const { return a[static_cast<std::size_t>(i) * cols + j]; }
    bool operator==(const Matrix& o) const {
        return rows == o.rows && cols == o.cols && a == o.a;
    }
};

// Classical Theta(n^3). Returns a 0x0 matrix if the sizes do not match.
// If `mults` is given, the number of scalar multiplications is added to it.
inline Matrix matMulClassical(const Matrix& A, const Matrix& B, long long* mults = nullptr) {
    if (A.cols != B.rows) return Matrix(0, 0);
    Matrix C(A.rows, B.cols);
    for (int i = 0; i < A.rows; ++i)
        for (int k = 0; k < A.cols; ++k) {
            long long aik = A.at(i, k);
            for (int j = 0; j < B.cols; ++j) C.at(i, j) += aik * B.at(k, j);
        }
    if (mults) *mults += static_cast<long long>(A.rows) * A.cols * B.cols;
    return C;
}

namespace detail {

inline Matrix matAdd(const Matrix& A, const Matrix& B) {
    Matrix C(A.rows, A.cols);
    for (std::size_t i = 0; i < A.a.size(); ++i) C.a[i] = A.a[i] + B.a[i];
    return C;
}

inline Matrix matSub(const Matrix& A, const Matrix& B) {
    Matrix C(A.rows, A.cols);
    for (std::size_t i = 0; i < A.a.size(); ++i) C.a[i] = A.a[i] - B.a[i];
    return C;
}

// h x h block of A whose top-left corner is (r0, c0)
inline Matrix getBlock(const Matrix& A, int r0, int c0, int h) {
    Matrix R(h, h);
    for (int i = 0; i < h; ++i)
        for (int j = 0; j < h; ++j) R.at(i, j) = A.at(r0 + i, c0 + j);
    return R;
}

inline void setBlock(Matrix& C, const Matrix& S, int r0, int c0) {
    for (int i = 0; i < S.rows; ++i)
        for (int j = 0; j < S.cols; ++j) C.at(r0 + i, c0 + j) = S.at(i, j);
}

// A and B are n x n with n a power of 2.  T(n) = 7T(n/2) + Theta(n^2)
inline Matrix strassenRec(const Matrix& A, const Matrix& B, int cutoff, long long* mults) {
    int n = A.rows;
    if (n <= cutoff) return matMulClassical(A, B, mults);
    int h = n / 2;
    Matrix A11 = getBlock(A, 0, 0, h), A12 = getBlock(A, 0, h, h);
    Matrix A21 = getBlock(A, h, 0, h), A22 = getBlock(A, h, h, h);
    Matrix B11 = getBlock(B, 0, 0, h), B12 = getBlock(B, 0, h, h);
    Matrix B21 = getBlock(B, h, 0, h), B22 = getBlock(B, h, h, h);

    // the 7 products
    Matrix M1 = strassenRec(matAdd(A11, A22), matAdd(B11, B22), cutoff, mults);
    Matrix M2 = strassenRec(matAdd(A21, A22), B11, cutoff, mults);
    Matrix M3 = strassenRec(A11, matSub(B12, B22), cutoff, mults);
    Matrix M4 = strassenRec(A22, matSub(B21, B11), cutoff, mults);
    Matrix M5 = strassenRec(matAdd(A11, A12), B22, cutoff, mults);
    Matrix M6 = strassenRec(matSub(A21, A11), matAdd(B11, B12), cutoff, mults);
    Matrix M7 = strassenRec(matSub(A12, A22), matAdd(B21, B22), cutoff, mults);

    Matrix C(n, n);
    setBlock(C, matAdd(matSub(matAdd(M1, M4), M5), M7), 0, 0);   // C11 = M1+M4-M5+M7
    setBlock(C, matAdd(M3, M5), 0, h);                           // C12 = M3+M5
    setBlock(C, matAdd(M2, M4), h, 0);                           // C21 = M2+M4
    setBlock(C, matAdd(matSub(matAdd(M1, M3), M2), M6), h, h);   // C22 = M1-M2+M3+M6
    return C;
}

} // namespace detail

// Strassen for any sizes: pads to a power of 2, multiplies, crops.
// Blocks of size <= cutoff are multiplied the classical way.
inline Matrix matMulStrassen(const Matrix& A, const Matrix& B, int cutoff = 1,
                             long long* mults = nullptr) {
    if (A.cols != B.rows) return Matrix(0, 0);
    if (A.rows == 0 || A.cols == 0 || B.cols == 0) return Matrix(A.rows, B.cols);
    if (cutoff < 1) cutoff = 1;
    int m = std::max(A.rows, std::max(A.cols, B.cols));
    int p = 1;
    while (p < m) p *= 2;

    Matrix PA(p, p), PB(p, p);
    for (int i = 0; i < A.rows; ++i)
        for (int j = 0; j < A.cols; ++j) PA.at(i, j) = A.at(i, j);
    for (int i = 0; i < B.rows; ++i)
        for (int j = 0; j < B.cols; ++j) PB.at(i, j) = B.at(i, j);

    Matrix PC = detail::strassenRec(PA, PB, cutoff, mults);

    Matrix C(A.rows, B.cols);
    for (int i = 0; i < A.rows; ++i)
        for (int j = 0; j < B.cols; ++j) C.at(i, j) = PC.at(i, j);
    return C;
}


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