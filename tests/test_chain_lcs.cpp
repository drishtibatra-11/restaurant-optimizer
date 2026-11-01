#include "dp.hpp"
#include "test.hpp"
#include <cctype>
#include <random>
#include <string>
#include <vector>

// ---------- oracles ----------
// Matrix chain: try every split without remembering anything (small n only)
static long long bruteChain(const std::vector<long long>& p, int i, int j) {
    if (i == j) return 0;
    long long best = -1;
    for (int k = i; k < j; ++k) {
        long long c = bruteChain(p, i, k) + bruteChain(p, k + 1, j) + p[i - 1] * p[k] * p[j];
        if (best < 0 || c < best) best = c;
    }
    return best;
}

struct Parsed { bool ok; long long r, c, cost; };

// Reads a bracketing like "((A1A2)A3)" and works out its real cost
static Parsed parseOrder(const std::string& s, std::size_t& pos, const std::vector<long long>& p) {
    Parsed bad{false, 0, 0, 0};
    if (pos >= s.size()) return bad;
    if (s[pos] == 'A') {
        ++pos;
        std::size_t idx = 0;
        bool digit = false;
        while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) {
            idx = idx * 10 + (s[pos] - '0');
            ++pos;
            digit = true;
        }
        if (!digit || idx < 1 || idx >= p.size()) return bad;
        return {true, p[idx - 1], p[idx], 0};
    }
    if (s[pos] != '(') return bad;
    ++pos;
    Parsed L = parseOrder(s, pos, p);
    Parsed R = parseOrder(s, pos, p);
    if (pos >= s.size() || s[pos] != ')') return bad;
    ++pos;
    if (!L.ok || !R.ok || L.c != R.r) return bad;
    return {true, L.r, R.c, L.cost + R.cost + L.r * L.c * R.c};
}

static bool orderOk(const std::vector<long long>& p, const ChainResult& res) {
    std::size_t pos = 0;
    Parsed x = parseOrder(res.order, pos, p);
    if (!x.ok || pos != res.order.size() || x.cost != res.cost) return false;
    std::string plain;                                  // brackets removed: must be A1A2...An
    for (char ch : res.order) if (ch != '(' && ch != ')') plain += ch;
    std::string expect;
    for (std::size_t i = 1; i < p.size(); ++i) expect += "A" + std::to_string(i);
    return plain == expect;
}

// LCS: try every subset of a (only for small n) and test if it is a subsequence of b
static bool isSubseq(const std::vector<int>& x, const std::vector<int>& y) {
    std::size_t j = 0;
    for (std::size_t i = 0; i < y.size() && j < x.size(); ++i) if (y[i] == x[j]) ++j;
    return j == x.size();
}

static std::size_t bruteLCS(const std::vector<int>& a, const std::vector<int>& b) {
    std::size_t best = 0;
    for (unsigned mask = 0; mask < (1u << a.size()); ++mask) {
        std::vector<int> sub;
        for (std::size_t i = 0; i < a.size(); ++i) if (mask >> i & 1) sub.push_back(a[i]);
        if (sub.size() > best && isSubseq(sub, b)) best = sub.size();
    }
    return best;
}

static std::vector<int> toVec(const std::string& s) {
    return std::vector<int>(s.begin(), s.end());
}

int main() {
    // ---------- matrix chain ----------
    CHECK(matrixChain({}).cost == 0 && matrixChain({5}).cost == 0);
    {
        ChainResult one = matrixChain({4, 7});                  // a single matrix
        CHECK(one.cost == 0 && one.order == "A1");
        ChainResult two = matrixChain({10, 20, 30});            // 10*20*30
        CHECK(two.cost == 6000 && two.order == "(A1A2)");
    }
    {   // textbook example: 30x35, 35x15, 15x5, 5x10, 10x20, 20x25 -> 15125
        std::vector<long long> p = {30, 35, 15, 5, 10, 20, 25};
        ChainResult r = matrixChain(p);
        CHECK(r.cost == 15125);
        CHECK(r.order == "((A1(A2A3))((A4A5)A6))");
        CHECK(orderOk(p, r));
        CHECK(chainCostLeftToRight(p) > r.cost);                // the best order really saves work
    }
    {   // 300 random chains against the brute force, and the order must be real
        std::mt19937 rng(81);
        bool costOk = true, ordOk = true, notWorse = true;
        for (int trial = 0; trial < 300; ++trial) {
            int n = 1 + static_cast<int>(rng() % 8);
            std::vector<long long> p(n + 1);
            for (long long& x : p) x = 1 + static_cast<long long>(rng() % 30);
            ChainResult r = matrixChain(p);
            if (r.cost != bruteChain(p, 1, n)) costOk = false;
            if (!orderOk(p, r)) ordOk = false;
            if (r.cost > chainCostLeftToRight(p)) notWorse = false;
        }
        CHECK(costOk);
        CHECK(ordOk);
        CHECK(notWorse);
    }

    // ---------- LCS ----------
    CHECK(lcs({}, {1, 2}).empty() && lcs({1, 2}, {}).empty() && lcsLength({}, {}) == 0);
    CHECK(lcs({1, 2, 3}, {4, 5, 6}).empty());                  // nothing in common
    CHECK(lcs({7, 7, 7}, {7, 7, 7}) == std::vector<int>({7, 7, 7}));
    {   // textbook example: ABCBDAB and BDCABA have an LCS of length 4
        std::vector<int> a = toVec("ABCBDAB"), b = toVec("BDCABA");
        std::vector<int> s = lcs(a, b);
        CHECK(s.size() == 4 && lcsLength(a, b) == 4);
        CHECK(isSubseq(s, a) && isSubseq(s, b));
    }
    {   // 300 random pairs: length matches the brute force, the sequence is real
        std::mt19937 rng(82);
        bool lenOk = true, seqOk = true, rowOk = true, symOk = true;
        for (int trial = 0; trial < 300; ++trial) {
            std::vector<int> a(rng() % 11), b(rng() % 11);
            for (int& x : a) x = static_cast<int>(rng() % 4);
            for (int& x : b) x = static_cast<int>(rng() % 4);
            std::vector<int> s = lcs(a, b);
            std::size_t expect = bruteLCS(a, b);
            if (s.size() != expect) lenOk = false;
            if (!isSubseq(s, a) || !isSubseq(s, b)) seqOk = false;
            if (lcsLength(a, b) != expect) rowOk = false;
            if (lcsLength(b, a) != expect) symOk = false;
        }
        CHECK(lenOk);
        CHECK(seqOk);
        CHECK(rowOk);
        CHECK(symOk);
    }
    {   // restaurant use: planned vs actual event sequence
        // 1 BOOKED, 2 SEATED, 3 ORDERED, 4 SERVED, 5 PAID
        std::vector<int> planned = {1, 2, 3, 4, 5};
        std::vector<int> actual  = {1, 2, 3, 3, 4, 5};     // the order was placed twice
        CHECK(lcs(planned, actual) == planned);            // nothing planned was skipped
        std::vector<int> skipped = {1, 2, 4, 5};           // the order step never happened
        CHECK(lcs(planned, skipped) == skipped);
        CHECK(lcsLength(planned, skipped) == 4);
    }

    return TEST_SUMMARY();
}