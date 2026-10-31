#include "csv_loader.hpp"
#include "dc.hpp"
#include "test.hpp"
#include <random>
#include <vector>

static long long rangeSum(const std::vector<int>& a, long lo, long hi) {
    long long s = 0;
    for (long i = lo; i <= hi; ++i) s += a[i];
    return s;
}

// true if the result is the true maximum and its indices are consistent
static bool resultOk(const std::vector<int>& a, const SubResult& r, long long expect) {
    return r.sum == expect && r.lo >= 0 && r.lo <= r.hi &&
           r.hi < static_cast<long>(a.size()) && rangeSum(a, r.lo, r.hi) == r.sum;
}

int main() {
    // edge cases
    CHECK(maxSubarrayDC({5}).sum == 5);
    CHECK(maxSubarrayKadane({-3}).sum == -3);
    CHECK(maxSubarrayDC({-4, -2, -9}).sum == -2);       // all negative
    CHECK(maxSubarrayKadane({-4, -2, -9}).sum == -2);
    CHECK(maxSubarrayDC({1, 2, 3}).sum == 6);           // all positive
    CHECK(maxSubarrayDC({0, 0, 0}).sum == 0);           // all zero

    // classic textbook example: best stretch is 4, -1, 2, 1 = 6
    {
        std::vector<int> a = {-2, 1, -3, 4, -1, 2, 1, -5, 4};
        SubResult d = maxSubarrayDC(a), k = maxSubarrayKadane(a);
        CHECK(d.sum == 6 && d.lo == 3 && d.hi == 6);
        CHECK(k.sum == 6 && k.lo == 3 && k.hi == 6);
    }

    // many random arrays against the brute force oracle
    {
        std::mt19937 rng(11);
        bool ok = true;
        for (int trial = 0; trial < 300; ++trial) {
            int n = 1 + static_cast<int>(rng() % 60);
            std::vector<int> a(n);
            for (int& x : a) x = static_cast<int>(rng() % 21) - 10;   // -10..10
            long long expect = maxSubarrayBrute(a);
            if (!resultOk(a, maxSubarrayDC(a), expect))     ok = false;
            if (!resultOk(a, maxSubarrayKadane(a), expect)) ok = false;
        }
        CHECK(ok);
    }

    // restaurant use: net guests per 15-minute slot, find the busiest build-up
    {
        Dataset d;
        std::string err;
        if (loadDataset("data/medium", d, err)) {
            std::vector<int> net(96, 0);                 // 96 slots of 15 minutes per day
            for (const Reservation& r : d.reservations) {
                int in  = r.start / 15;
                int out = r.end() / 15;
                if (in < 96)  net[in]  += r.party;
                if (out < 96) net[out] -= r.party;
            }
            SubResult dc = maxSubarrayDC(net), kd = maxSubarrayKadane(net);
            CHECK(dc.sum == kd.sum);
            CHECK(dc.sum == maxSubarrayBrute(net));
            CHECK(resultOk(net, dc, dc.sum));
        } else {
            CHECK(false);  // run "make data" first
        }
    }

    return TEST_SUMMARY();
}