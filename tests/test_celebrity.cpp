#include "dc.hpp"
#include "test.hpp"
#include <random>
#include <vector>

// Random group where `celeb` (if >= 0) is the celebrity.
static KnowsMatrix makeGroup(int n, int celeb, std::mt19937& rng) {
    KnowsMatrix k(n, std::vector<bool>(n, false));
    for (int a = 0; a < n; ++a)
        for (int b = 0; b < n; ++b)
            if (a != b) k[a][b] = rng() % 2;
    if (celeb >= 0) {
        for (int i = 0; i < n; ++i) {
            if (i == celeb) continue;
            k[i][celeb] = true;      // everyone knows the celebrity
            k[celeb][i] = false;     // the celebrity knows nobody
        }
    }
    return k;
}

int main() {
    std::mt19937 rng(21);

    // edge cases
    {
        KnowsMatrix empty;
        CHECK(celebrityBrute(empty).person == -1);
        CHECK(celebrityElim(empty).person == -1);
        CHECK(celebrityDC(empty).person == -1);

        KnowsMatrix one(1, std::vector<bool>(1, false));   // a single person is a celebrity
        CHECK(celebrityBrute(one).person == 0);
        CHECK(celebrityElim(one).person == 0);
        CHECK(celebrityDC(one).person == 0);
    }

    // a celebrity in every position, for many group sizes
    {
        bool ok = true;
        for (int n = 2; n <= 20; ++n)
            for (int c = 0; c < n; ++c) {
                KnowsMatrix k = makeGroup(n, c, rng);
                if (celebrityBrute(k).person != c) ok = false;
                if (celebrityElim(k).person != c)  ok = false;
                if (celebrityDC(k).person != c)    ok = false;
            }
        CHECK(ok);
    }

    // random groups (usually no celebrity): all three methods must agree
    {
        bool ok = true;
        for (int trial = 0; trial < 300; ++trial) {
            int n = 2 + static_cast<int>(rng() % 12);
            KnowsMatrix k = makeGroup(n, -1, rng);
            int expect = celebrityBrute(k).person;
            if (celebrityElim(k).person != expect) ok = false;
            if (celebrityDC(k).person != expect)   ok = false;
        }
        CHECK(ok);
    }

    // everyone knows everyone: no celebrity
    {
        int n = 6;
        KnowsMatrix k(n, std::vector<bool>(n, true));
        CHECK(celebrityElim(k).person == -1);
        CHECK(celebrityDC(k).person == -1);
        CHECK(celebrityBrute(k).person == -1);
    }

    // number of reads: linear for elimination and divide and conquer
    {
        int n = 200;
        KnowsMatrix k = makeGroup(n, n - 1, rng);   // celebrity is the last person
        CelebResult e = celebrityElim(k), d = celebrityDC(k), b = celebrityBrute(k);
        CHECK(e.person == n - 1 && d.person == n - 1 && b.person == n - 1);
        CHECK(e.calls <= 3LL * (n - 1));
        CHECK(d.calls <= 3LL * (n - 1) + 1);
        CHECK(b.calls > e.calls);                   // brute force reads more
    }

    return TEST_SUMMARY();
}