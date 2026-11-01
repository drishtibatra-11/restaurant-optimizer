#include "backtrack.hpp"
#include "csv_loader.hpp"
#include "test.hpp"
#include <algorithm>
#include <cstdlib>
#include <numeric>
#include <random>
#include <set>
#include <string>
#include <vector>

// no two queens share a column or a diagonal (rows are different by construction)
static bool boardValid(const std::vector<int>& col, int n) {
    if (static_cast<int>(col.size()) != n) return false;
    for (int i = 0; i < n; ++i) {
        if (col[i] < 0 || col[i] >= n) return false;
        for (int j = i + 1; j < n; ++j)
            if (col[i] == col[j] || std::abs(col[i] - col[j]) == j - i) return false;
    }
    return true;
}

// Oracle: try every ordering of the columns (n! boards) and keep the valid ones
static long long bruteQueens(int n) {
    std::vector<int> p(n);
    std::iota(p.begin(), p.end(), 0);
    long long count = 0;
    do { if (boardValid(p, n)) ++count; } while (std::next_permutation(p.begin(), p.end()));
    return count;
}

// Oracle: try every subset
static long long bruteSubset(const std::vector<int>& a, long long target) {
    long long count = 0;
    for (unsigned long long mask = 0; mask < (1ULL << a.size()); ++mask) {
        long long s = 0;
        for (std::size_t i = 0; i < a.size(); ++i) if (mask >> i & 1) s += a[i];
        if (s == target) ++count;
    }
    return count;
}

static bool firstOk(const std::vector<int>& a, const SubsetResult& r, long long target) {
    std::set<std::size_t> seen;
    long long s = 0;
    for (std::size_t i : r.first) {
        if (i >= a.size() || !seen.insert(i).second) return false;
        s += a[i];
    }
    return s == target;
}

int main() {
    // ---------- N-Queens ----------
    CHECK(nQueens(0).solutions == 0 && nQueens(-3).solutions == 0);
    CHECK(nQueens(1).solutions == 1 && nQueens(2).solutions == 0 && nQueens(3).solutions == 0);
    {   // known numbers of solutions
        const long long known[] = {0, 1, 0, 0, 2, 10, 4, 40, 92, 352, 724, 2680, 14200};
        bool ok = true;
        for (int n = 1; n <= 12; ++n) if (nQueens(n).solutions != known[n]) ok = false;
        CHECK(ok);
    }
    {   // the first board must really be valid, and stopping early must look at fewer nodes
        bool valid = true, fewer = true;
        for (int n = 4; n <= 10; ++n) {
            QueensResult all = nQueens(n), one = nQueens(n, true);
            if (!boardValid(all.first, n) || !boardValid(one.first, n)) valid = false;
            if (one.solutions != 1 || one.nodes >= all.nodes) fewer = false;
        }
        CHECK(valid);
        CHECK(fewer);
        CHECK(nQueens(3, true).first.empty());            // no board exists
    }
    {   // against the brute force over all column orders
        bool ok = true;
        for (int n = 1; n <= 8; ++n) if (nQueens(n).solutions != bruteQueens(n)) ok = false;
        CHECK(ok);
    }
    {   // backtracking looks at far fewer boards than trying all n^n placements
        QueensResult r = nQueens(8);
        CHECK(r.nodes < 20000);                           // 8^8 = 16,777,216
        std::cout << "  8 queens: " << r.solutions << " solutions, " << r.nodes
                  << " boards looked at (8^8 = 16777216 without pruning)\n";
    }

    // ---------- subset sum ----------
    CHECK(subsetSum({}, 0).count == 1);                   // the empty subset
    CHECK(subsetSum({}, 5).count == 0);
    CHECK(!subsetSum({3, -1}, 2).ok);                     // negative items are refused
    {   // hand example: {3,34,4,12,5,2}, target 9 -> {4,5} and {3,4,2}
        std::vector<int> a = {3, 34, 4, 12, 5, 2};
        SubsetResult r = subsetSum(a, 9);
        CHECK(r.ok && r.count == 2 && firstOk(a, r, 9));
        CHECK(subsetSum(a, 100).count == 0);
        CHECK(subsetSum(a, 0).count == 1);
    }
    {   // zeros double the number of subsets: {0,0,5} -> 4 ways to make 5
        CHECK(subsetSum({0, 0, 5}, 5).count == 4);
        CHECK(subsetSum({0, 0, 0}, 0).count == 8);
    }
    {   // stopping at the first solution
        std::vector<int> a = {3, 34, 4, 12, 5, 2};
        SubsetResult one = subsetSum(a, 9, true), all = subsetSum(a, 9);
        CHECK(one.count == 1 && firstOk(a, one, 9) && one.nodes < all.nodes);
    }
    {   // 300 random cases against the brute force
        std::mt19937 rng(121);
        bool countOk = true, firstGood = true, stopOk = true;
        for (int trial = 0; trial < 300; ++trial) {
            std::vector<int> a(rng() % 13);
            for (int& x : a) x = static_cast<int>(rng() % 12);
            long long target = static_cast<long long>(rng() % 40);
            SubsetResult r = subsetSum(a, target);
            long long expect = bruteSubset(a, target);
            if (!r.ok || r.count != expect) countOk = false;
            if (expect > 0 && !firstOk(a, r, target)) firstGood = false;
            SubsetResult s = subsetSum(a, target, true);
            if (s.count != (expect > 0 ? 1 : 0)) stopOk = false;
        }
        CHECK(countOk);
        CHECK(firstGood);
        CHECK(stopOk);
    }

    // restaurant use: which dishes can fill exactly 60 minutes of one station's time?
    {
        Dataset d;
        std::string err;
        if (loadDataset("data/medium", d, err)) {
            std::vector<int> prep;
            for (const Dish& x : d.dishes) prep.push_back(x.prepTime);
            SubsetResult r = subsetSum(prep, 60), one = subsetSum(prep, 60, true);
            CHECK(r.count == bruteSubset(prep, 60));
            CHECK(one.count == (r.count > 0 ? 1 : 0));
            CHECK(r.count == 0 || firstOk(prep, one, 60));
            std::cout << "  exact 60 minutes from " << prep.size() << " dishes: " << r.count
                      << " ways (" << r.nodes << " nodes; " << (1LL << (prep.size() + 1)) - 1
                      << " without pruning)\n";
        } else {
            CHECK(false);  // run "make data" first
        }
    }

    return TEST_SUMMARY();
}