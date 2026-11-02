#pragma once
#include <cstddef>
#include <string>
#include <vector>

// =====================================================================
// String matching. Every function returns ALL start positions (overlapping
// matches included). An empty pattern, or a pattern longer than the text,
// gives no matches. If `cmps` is given, the work done is added to it:
// character comparisons (naive, Rabin-Karp, KMP) or table steps (automaton).
// =====================================================================
using Positions = std::vector<std::size_t>;

// Naive: try every start. Theta(n*m) worst case.
inline Positions matchNaive(const std::string& t, const std::string& p, long long* cmps = nullptr) {
    Positions out;
    std::size_t n = t.size(), m = p.size();
    if (m == 0 || m > n) return out;
    for (std::size_t i = 0; i + m <= n; ++i) {
        std::size_t j = 0;
        while (j < m) {
            if (cmps) ++*cmps;
            if (t[i + j] != p[j]) break;
            ++j;
        }
        if (j == m) out.push_back(i);
    }
    return out;
}

// Rabin-Karp: compare rolling hashes and check the characters only when the hashes agree.
// Theta(n + m) on average, Theta(n*m) if every window collides.
inline Positions matchRabinKarp(const std::string& t, const std::string& p, long long* cmps = nullptr) {
    Positions out;
    std::size_t n = t.size(), m = p.size();
    if (m == 0 || m > n) return out;
    const unsigned long long MOD = 1000000007ULL, BASE = 257ULL;
    unsigned long long hp = 0, ht = 0, high = 1;           // high = BASE^(m-1) mod MOD
    for (std::size_t i = 0; i + 1 < m; ++i) high = high * BASE % MOD;
    for (std::size_t i = 0; i < m; ++i) {
        hp = (hp * BASE + static_cast<unsigned char>(p[i])) % MOD;
        ht = (ht * BASE + static_cast<unsigned char>(t[i])) % MOD;
    }
    for (std::size_t i = 0;; ++i) {
        if (hp == ht) {                                    // same hash: verify (collisions happen)
            std::size_t j = 0;
            while (j < m) {
                if (cmps) ++*cmps;
                if (t[i + j] != p[j]) break;
                ++j;
            }
            if (j == m) out.push_back(i);
        }
        if (i + m >= n) break;
        ht = (ht + MOD - static_cast<unsigned char>(t[i]) * high % MOD) % MOD;     // drop t[i]
        ht = (ht * BASE + static_cast<unsigned char>(t[i + m])) % MOD;             // add t[i+m]
    }
    return out;
}

// Finite automaton: build a table once (Theta(m * 256)), then read the text one character
// at a time with no backing up (Theta(n)).
inline Positions matchAutomaton(const std::string& t, const std::string& p, long long* cmps = nullptr) {
    Positions out;
    std::size_t n = t.size(), m = p.size();
    if (m == 0 || m > n) return out;
    std::vector<std::vector<int>> delta(m + 1, std::vector<int>(256, 0));
    delta[0][static_cast<unsigned char>(p[0])] = 1;
    std::size_t x = 0;                                     // state reached by p[1..j-1]
    for (std::size_t j = 1; j <= m; ++j) {
        delta[j] = delta[x];
        if (j < m) {
            delta[j][static_cast<unsigned char>(p[j])] = static_cast<int>(j + 1);
            x = static_cast<std::size_t>(delta[x][static_cast<unsigned char>(p[j])]);
        }
    }
    std::size_t state = 0;
    for (std::size_t i = 0; i < n; ++i) {
        if (cmps) ++*cmps;
        state = static_cast<std::size_t>(delta[state][static_cast<unsigned char>(t[i])]);
        if (state == m) out.push_back(i + 1 - m);
    }
    return out;
}

// Knuth-Morris-Pratt: the failure table says how far to fall back after a mismatch,
// so the text is never re-read. Theta(n + m).
inline Positions matchKMP(const std::string& t, const std::string& p, long long* cmps = nullptr) {
    Positions out;
    std::size_t n = t.size(), m = p.size();
    if (m == 0 || m > n) return out;
    std::vector<std::size_t> fail(m, 0);                   // fail[i] = longest border of p[0..i]
    for (std::size_t i = 1, k = 0; i < m; ++i) {
        while (k > 0 && p[i] != p[k]) { if (cmps) ++*cmps; k = fail[k - 1]; }
        if (cmps) ++*cmps;
        if (p[i] == p[k]) ++k;
        fail[i] = k;
    }
    for (std::size_t i = 0, k = 0; i < n; ++i) {
        while (k > 0 && t[i] != p[k]) { if (cmps) ++*cmps; k = fail[k - 1]; }
        if (cmps) ++*cmps;
        if (t[i] == p[k]) ++k;
        if (k == m) { out.push_back(i + 1 - m); k = fail[k - 1]; }
    }
    return out;
}