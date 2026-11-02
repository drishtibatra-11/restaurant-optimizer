#pragma once
#include <climits>
#include <vector>

// =====================================================================
// Number theory: extended Euclid, modular inverse, Chinese Remainder Theorem
// =====================================================================

// Returns g = gcd(a, b) and finds x, y with a*x + b*y = g.
inline long long extGcd(long long a, long long b, long long& x, long long& y) {
    if (b == 0) { x = 1; y = 0; return a; }
    long long x1, y1;
    long long g = extGcd(b, a % b, x1, y1);
    x = y1;
    y = x1 - (a / b) * y1;
    return g;
}

// Inverse of a modulo m (m > 0). Returns false if a and m are not coprime.
inline bool modInverse(long long a, long long m, long long& inv) {
    if (m <= 0) return false;
    long long x, y;
    long long g = extGcd(((a % m) + m) % m, m, x, y);
    if (g != 1) return false;
    inv = ((x % m) + m) % m;
    return true;
}

struct CRTResult {
    bool ok = false;      // false: no solution, bad input, or the combined modulus is too large
    long long x = 0;      // the smallest non-negative solution
    long long mod = 1;    // all solutions are x + k * mod (the lcm of the moduli)
};

// Solves x = r[i] (mod m[i]) for all i. The moduli do not have to be coprime.
inline CRTResult crt(const std::vector<long long>& r, const std::vector<long long>& m) {
    CRTResult res;
    if (r.size() != m.size()) return res;
    long long x = 0, mod = 1;
    for (std::size_t i = 0; i < r.size(); ++i) {
        if (m[i] <= 0) return CRTResult();
        long long ri = ((r[i] % m[i]) + m[i]) % m[i];
        long long p, q;
        long long g = extGcd(mod, m[i], p, q);               // mod*p + m[i]*q = g
        if ((ri - x) % g != 0) return CRTResult();           // the two conditions clash
        long long step = m[i] / g;
        __int128 lcm = static_cast<__int128>(mod) / g * m[i];
        if (lcm > LLONG_MAX) return CRTResult();
        long long k = static_cast<long long>(
            ((static_cast<__int128>((ri - x) / g) * p) % step + step) % step);
        x = static_cast<long long>((x + static_cast<__int128>(mod) * k) % lcm);
        mod = static_cast<long long>(lcm);
    }
    res.ok = true;
    res.x = x;
    res.mod = mod;
    return res;
}