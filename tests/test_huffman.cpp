#include "csv_loader.hpp"
#include "huffman.hpp"
#include "test.hpp"
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

// Independent oracle: the optimal cost is the sum of all merge weights,
// found by repeatedly taking the two smallest weights.
static unsigned long long oracleCost(const std::string& text) {
    std::map<unsigned char, unsigned long long> f;
    for (unsigned char c : text) ++f[c];
    std::multiset<unsigned long long> w;
    for (auto& kv : f) w.insert(kv.second);
    if (w.empty()) return 0;
    if (w.size() == 1) return *w.begin();            // 1 bit per symbol
    unsigned long long cost = 0;
    while (w.size() > 1) {
        unsigned long long a = *w.begin(); w.erase(w.begin());
        unsigned long long b = *w.begin(); w.erase(w.begin());
        cost += a + b;
        w.insert(a + b);
    }
    return cost;
}

static bool prefixFree(const HuffmanTable& t) {
    for (int a = 0; a < 256; ++a) {
        if (t.code[a].empty()) continue;
        for (int b = 0; b < 256; ++b) {
            if (a == b || t.code[b].empty()) continue;
            if (t.code[b].compare(0, t.code[a].size(), t.code[a]) == 0) return false;
        }
    }
    return true;
}

static double kraftSum(const HuffmanTable& t) {
    double s = 0;
    for (int a = 0; a < 256; ++a)
        if (!t.code[a].empty()) {
            double x = 1;
            for (std::size_t i = 0; i < t.code[a].size(); ++i) x /= 2;
            s += x;
        }
    return s;
}

static bool roundTrip(const std::string& text) {
    HuffmanTable t = buildHuffman(text);
    std::string bits = huffmanEncode(text, t), back;
    if (!huffmanDecode(bits, t, back) || back != text) return false;
    if (bits.size() != huffmanBits(t)) return false;
    return unpackBits(packBits(bits), bits.size()) == bits;
}

static std::string randomText(std::mt19937& rng, int len, int alphabet, bool skewed) {
    std::string s;
    for (int i = 0; i < len; ++i) {
        int x = static_cast<int>(rng() % alphabet);
        if (skewed) x = std::min(x, static_cast<int>(rng() % alphabet));
        s += static_cast<char>(x);
    }
    return s;
}

int main() {
    // empty text
    {
        HuffmanTable t = buildHuffman("");
        CHECK(t.root == -1 && t.distinct == 0);
        std::string out = "x";
        CHECK(huffmanEncode("", t).empty());
        CHECK(huffmanDecode("", t, out) && out.empty());
        CHECK(!huffmanDecode("0", t, out));
    }

    // a single distinct symbol gets the 1-bit code "0"
    {
        HuffmanTable t = buildHuffman("aaaa");
        CHECK(t.code['a'] == "0");
        CHECK(huffmanEncode("aaaa", t) == "0000");
        CHECK(roundTrip("aaaa"));
        CHECK(roundTrip("z"));
    }

    // textbook example: a5 b2 r2 c1 d1 needs exactly 23 bits
    {
        HuffmanTable t = buildHuffman("abracadabra");
        CHECK(huffmanBits(t) == 23);
        CHECK(huffmanEncode("abracadabra", t).size() == 23);
        CHECK(t.code['a'].size() < t.code['c'].size());   // frequent symbol, shorter code
        CHECK(roundTrip("abracadabra"));
    }

    // prefix-free and Kraft sum = 1 (a full binary tree)
    {
        HuffmanTable t = buildHuffman("the quick brown fox jumps over the lazy dog");
        CHECK(prefixFree(t));
        double k = kraftSum(t);
        CHECK(k > 0.999999 && k < 1.000001);
    }

    // 300 random texts: optimal against the oracle, never worse than fixed-length, lossless
    {
        std::mt19937 rng(41);
        bool optimal = true, notWorse = true, lossless = true, prefix = true;
        for (int trial = 0; trial < 300; ++trial) {
            int alphabet = 1 + static_cast<int>(rng() % 256);
            int len = 1 + static_cast<int>(rng() % 400);
            std::string s = randomText(rng, len, alphabet, trial % 2 == 0);
            HuffmanTable t = buildHuffman(s);
            if (huffmanBits(t) != oracleCost(s)) optimal = false;
            if (t.distinct >= 2) {
                int per = 1;
                while ((1u << per) < t.distinct) ++per;       // ceil(log2(distinct))
                if (huffmanBits(t) > static_cast<unsigned long long>(per) * s.size())
                    notWorse = false;
            }
            if (!roundTrip(s)) lossless = false;
            if (!prefixFree(t)) prefix = false;
        }
        CHECK(optimal);
        CHECK(notWorse);
        CHECK(lossless);
        CHECK(prefix);
    }

    // corrupt input is rejected
    {
        HuffmanTable t = buildHuffman("abcd");         // four equal symbols: all codes have 2 bits
        std::string bits = huffmanEncode("abcd", t), out;
        CHECK(bits.size() == 8);
        CHECK(huffmanDecode(bits, t, out) && out == "abcd");
        CHECK(!huffmanDecode(bits.substr(0, 7), t, out));   // ends in the middle of a code
        CHECK(!huffmanDecode("01x0", t, out));              // not a bit
    }

    // real event log from the medium dataset: lossless and smaller
    {
        Dataset d;
        std::string err;
        if (loadDataset("data/medium", d, err)) {
            std::string log = buildEventLog(d);
            HuffmanTable t = buildHuffman(log);
            std::string bits = huffmanEncode(log, t), back;
            CHECK(huffmanDecode(bits, t, back) && back == log);
            std::string packed = packBits(bits);
            CHECK(packed.size() < log.size());
            std::cout << "  event log: " << log.size() << " bytes -> " << packed.size()
                      << " bytes (" << 100 * packed.size() / log.size() << "% of original)\n";
        } else {
            CHECK(false);  // run "make data" first
        }
    }

    // adversarial: 10000 identical reservations
    {
        Dataset d;
        std::string err;
        if (loadDataset("data/adversarial/equal", d, err)) {
            CHECK(roundTrip(buildEventLog(d)));
        } else {
            CHECK(false);  // run "make data" first
        }
    }

    return TEST_SUMMARY();
}