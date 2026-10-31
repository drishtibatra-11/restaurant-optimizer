#include "csv_loader.hpp"
#include "dc.hpp"
#include "test.hpp"
#include <vector>

// Slow reference: multiply one at a time, reducing mod m each step
static unsigned long long slowPowMod(unsigned long long b, unsigned e, unsigned long long m) {
    unsigned long long r = 1 % m;
    for (unsigned i = 0; i < e; ++i) r = (r * (b % m)) % m;
    return r;
}

int main() {
    // known values and special cases
    CHECK(powerFast(2, 10) == 1024);
    CHECK(powerIter(2, 10) == 1024);
    CHECK(powerFast(7, 0) == 1);        // anything^0 = 1
    CHECK(powerFast(0, 5) == 0);
    CHECK(powerFast(0, 0) == 1);        // convention
    CHECK(powerFast(1, 60) == 1);
    CHECK(powerFast(2, 63) == 9223372036854775808ULL);

    // fast versions against the naive loop (results stay below 2^64)
    {
        bool ok = true;
        for (unsigned long long b = 0; b <= 9; ++b)
            for (unsigned e = 0; e <= 19; ++e) {
                unsigned long long expect = powerNaive(b, e);
                if (powerFast(b, e) != expect) ok = false;
                if (powerIter(b, e) != expect) ok = false;
            }
        CHECK(ok);
    }

    // modular power against the slow reference (small modulus, so no overflow)
    {
        bool ok = true;
        for (unsigned long long m : {1ULL, 2ULL, 7ULL, 97ULL, 1000ULL})
            for (unsigned long long b = 0; b < 30; ++b)
                for (unsigned e = 0; e <= 40; ++e)
                    if (powMod(b, e, m) != slowPowMod(b, e, m)) ok = false;
        CHECK(ok);
    }
    CHECK(powMod(5, 3, 1) == 0);
    CHECK(powMod(2, 10, 1000) == 24);   // 1024 mod 1000

    // huge exponent and modulus: Fermat's little theorem, a^(p-1) = 1 mod p
    {
        const unsigned long long p = 1000000007ULL;
        bool ok = true;
        for (unsigned long long a = 1; a <= 100; ++a)
            if (powMod(a, p - 1, p) != 1) ok = false;
        CHECK(ok);
        CHECK(powMod(2, 1ULL << 62, p) < p);   // runs fast and stays in range
    }

    // restaurant use: reservation checksums
    {
        Reservation a{1, 4, 1110, 90, 0};
        Reservation b{2, 4, 1110, 90, 0};
        CHECK(reservationCode(a) == reservationCode(a));   // deterministic
        CHECK(reservationCode(a) != reservationCode(b));   // detects a changed id
        Dataset d;
        std::string err;
        if (loadDataset("data/medium", d, err)) {
            bool inRange = true;
            for (const Reservation& r : d.reservations)
                if (reservationCode(r) >= 1000000007ULL) inRange = false;
            CHECK(inRange);
        } else {
            CHECK(false);  // run "make data" first
        }
    }

    return TEST_SUMMARY();
}