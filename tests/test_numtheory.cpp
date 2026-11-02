#include "numtheory.hpp"
#include "test.hpp"
#include <random>
#include <vector>

static long long gcdOf(long long a, long long b) { return b == 0 ? a : gcdOf(b, a % b); }

// Oracle: look at every number below the lcm
static bool bruteCRT(const std::vector<long long>& r, const std::vector<long long>& m,
                     long long lcm, long long& first) {
    for (long long x = 0; x < lcm; ++x) {
        bool all = true;
        for (std::size_t i = 0; i < r.size(); ++i)
            if (((x - r[i]) % m[i] + m[i]) % m[i] != 0) { all = false; break; }
        if (all) { first = x; return true; }
    }
    return false;
}

int main() {
    // extended Euclid and inverses
    {
        long long x, y;
        CHECK(extGcd(240, 46, x, y) == 2 && 240 * x + 46 * y == 2);
        long long inv;
        CHECK(modInverse(3, 11, inv) && inv == 4);                 // 3*4 = 12 = 1 (mod 11)
        CHECK(!modInverse(6, 9, inv));                             // not coprime
        CHECK(!modInverse(3, 0, inv));
        CHECK(modInverse(-3, 11, inv) && (inv * (-3 + 11)) % 11 == 1);
        bool ok = true;
        for (long long a = 1; a < 50; ++a)
            if (modInverse(a, 97, inv) && (a * inv) % 97 != 1) ok = false;
        CHECK(ok);
    }

    // classic example: x = 2 (mod 3), 3 (mod 5), 2 (mod 7)  ->  23 (mod 105)
    {
        CRTResult r = crt({2, 3, 2}, {3, 5, 7});
        CHECK(r.ok && r.x == 23 && r.mod == 105);
    }

    // no conditions, one condition, residues that need normalising
    {
        CHECK(crt({}, {}).ok && crt({}, {}).x == 0 && crt({}, {}).mod == 1);
        CRTResult one = crt({-1}, {7});
        CHECK(one.ok && one.x == 6 && one.mod == 7);
        CHECK(!crt({1}, {0}).ok && !crt({1}, {-3}).ok);             // bad modulus
        CHECK(!crt({1, 2}, {3}).ok);                               // sizes differ
    }

    // moduli that are not coprime: consistent and inconsistent
    {
        CRTResult a = crt({1, 3}, {4, 6});
        CHECK(a.ok && a.x == 9 && a.mod == 12);
        CHECK(!crt({0, 1}, {4, 6}).ok);                            // 0 (mod 4) and 1 (mod 6) clash
    }

    // the combined modulus does not fit in 64 bits
    {
        CHECK(!crt({1, 1}, {3000000019LL, 3000000037LL}).ok == false);   // about 9e18, still fits
        CHECK(!crt({1, 1, 1}, {3000000019LL, 3000000037LL, 3000000047LL}).ok);
    }

    // 300 random systems against the brute force
    {
        std::mt19937 rng(151);
        bool ok = true;
        for (int trial = 0; trial < 300; ++trial) {
            int k = 1 + static_cast<int>(rng() % 3);
            std::vector<long long> r(k), m(k);
            long long lcm = 1;
            for (int i = 0; i < k; ++i) {
                m[i] = 1 + static_cast<long long>(rng() % 12);
                r[i] = static_cast<long long>(rng() % 30) - 5;
                lcm = lcm / gcdOf(lcm, m[i]) * m[i];
            }
            long long expect = 0;
            bool has = bruteCRT(r, m, lcm, expect);
            CRTResult got = crt(r, m);
            if (got.ok != has) ok = false;
            else if (has && (got.x != expect || got.mod != lcm)) ok = false;
        }
        CHECK(ok);
    }

    // restaurant use: staff rotations. Waiter works every 3rd day, chef every 4th, cleaner every 5th.
    // First day when all three are on duty after starting on days 1, 2, 3 of their cycles.
    {
        CRTResult r = crt({1, 2, 3}, {3, 4, 5});
        CHECK(r.ok && r.mod == 60);
        CHECK((r.x - 1) % 3 == 0 && (r.x - 2) % 4 == 0 && (r.x - 3) % 5 == 0);
    }

    return TEST_SUMMARY();
}