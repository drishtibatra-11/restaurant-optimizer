#include "csv_loader.hpp"
#include "huffman.hpp"
#include "strmatch.hpp"
#include "test.hpp"
#include <random>
#include <string>
#include <vector>

// Oracle: the standard library search, repeated from every position
static Positions stdAll(const std::string& t, const std::string& p) {
    Positions out;
    if (p.empty()) return out;
    for (std::size_t pos = t.find(p); pos != std::string::npos; pos = t.find(p, pos + 1))
        out.push_back(pos);
    return out;
}

static bool allAgree(const std::string& t, const std::string& p) {
    Positions expect = stdAll(t, p);
    return matchNaive(t, p) == expect && matchRabinKarp(t, p) == expect &&
           matchAutomaton(t, p) == expect && matchKMP(t, p) == expect;
}

int main() {
    // edge cases: empty pattern, empty text, pattern longer than the text
    CHECK(matchNaive("abc", "").empty() && matchKMP("abc", "").empty());
    CHECK(matchRabinKarp("", "a").empty() && matchAutomaton("", "a").empty());
    CHECK(matchNaive("ab", "abc").empty() && matchKMP("ab", "abc").empty());
    CHECK(matchRabinKarp("ab", "abc").empty() && matchAutomaton("ab", "abc").empty());

    // simple cases, overlapping matches, whole-text match, single character
    {
        CHECK(allAgree("aaaa", "aa"));
        CHECK(matchKMP("aaaa", "aa") == Positions({0, 1, 2}));
        CHECK(matchNaive("abababab", "abab") == Positions({0, 2, 4}));
        CHECK(matchAutomaton("abracadabra", "abra") == Positions({0, 7}));
        CHECK(matchRabinKarp("hello", "hello") == Positions({0}));
        CHECK(matchKMP("banana", "a") == Positions({1, 3, 5}));
        CHECK(matchKMP("banana", "x").empty());
        CHECK(allAgree("abcabcabd", "abcabd"));
    }

    // binary data: zero bytes and bytes above 127 (the automaton uses an unsigned index)
    {
        std::string t("a\0b\xff\xfe\0b\xff", 8), p("\0b\xff", 3);
        CHECK(allAgree(t, p));
        CHECK(matchAutomaton(t, p) == Positions({1, 5}));
    }

    // 600 random strings over small alphabets: all four agree with the standard library
    {
        std::mt19937 rng(161);
        bool ok = true;
        for (int trial = 0; trial < 600; ++trial) {
            int alpha = 1 + static_cast<int>(rng() % 3);
            std::string t(rng() % 40, 'a'), p(1 + rng() % 5, 'a');
            for (char& c : t) c = static_cast<char>('a' + rng() % alpha);
            for (char& c : p) c = static_cast<char>('a' + rng() % alpha);
            if (!allAgree(t, p)) ok = false;
        }
        CHECK(ok);
    }

    // adversarial text: the naive method does about n*m work, KMP and the automaton about n
    {
        std::string t(20000, 'a'), p = std::string(50, 'a') + "b";
        long long cn = 0, ck = 0, ca = 0, cr = 0;
        CHECK(matchNaive(t, p, &cn).empty() && matchKMP(t, p, &ck).empty());
        CHECK(matchAutomaton(t, p, &ca).empty() && matchRabinKarp(t, p, &cr).empty());
        CHECK(ck <= 2LL * static_cast<long long>(t.size() + p.size()));   // at most 2(n+m)
        CHECK(ca == static_cast<long long>(t.size()));
        CHECK(cn > 10 * ck);
        // when the pattern is all 'a', every window matches
        std::string q(50, 'a');
        CHECK(matchKMP(t, q).size() == t.size() - 50 + 1);
        CHECK(allAgree(t, q));
    }

    // Rabin-Karp must survive hash collisions: it verifies every candidate
    {
        std::string t(5000, 'a');
        for (std::size_t i = 0; i < t.size(); i += 7) t[i] = 'b';
        CHECK(allAgree(t, "baaaaaab"));
        CHECK(allAgree(t, "aab"));
    }

    // restaurant use: search the event log
    {
        Dataset d;
        std::string err;
        if (loadDataset("data/medium", d, err)) {
            std::string log = buildEventLog(d);
            long long cn = 0, cr = 0, ca = 0, ck = 0;
            Positions n = matchNaive(log, "ORDER_PLACED", &cn);
            CHECK(n.size() == d.orders.size());
            CHECK(matchRabinKarp(log, "ORDER_PLACED", &cr) == n);
            CHECK(matchAutomaton(log, "ORDER_PLACED", &ca) == n);
            CHECK(matchKMP(log, "ORDER_PLACED", &ck) == n);
            CHECK(matchKMP(log, "BOOKED id=").size() == d.reservations.size());
            CHECK(matchKMP(log, "ORDER_CANCEL").empty());
            std::cout << "  \"ORDER_PLACED\" in a " << log.size() << "-byte log: " << n.size()
                      << " matches; work naive " << cn << ", Rabin-Karp " << cr << ", automaton " << ca
                      << ", KMP " << ck << "\n";
        } else {
            CHECK(false);  // run "make data" first
        }
    }

    return TEST_SUMMARY();
}