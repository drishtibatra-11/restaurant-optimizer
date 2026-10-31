#pragma once
#include "model.hpp"
#include <cstddef>
#include <functional>
#include <queue>
#include <string>
#include <utility>
#include <vector>

// ---------- Huffman coding: optimal prefix code, greedy, Theta(n log n) ----------
struct HNode {
    unsigned long long w;   // weight (frequency)
    int sym;                // byte value for a leaf, -1 for an internal node
    int left;               // child reached by bit '0'
    int right;              // child reached by bit '1'
};

struct HuffmanTable {
    std::vector<HNode> nodes;          // the first `distinct` nodes are the leaves
    int root = -1;                     // -1 when the text is empty
    std::vector<std::string> code = std::vector<std::string>(256);  // "0101" per byte
    std::size_t distinct = 0;          // number of different byte values
};

inline HuffmanTable buildHuffman(const std::string& text) {
    HuffmanTable t;
    unsigned long long freq[256] = {0};
    for (unsigned char c : text) ++freq[c];

    // min-heap of (weight, node index); the index breaks ties, so the result is deterministic
    using Item = std::pair<unsigned long long, int>;
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> pq;
    for (int s = 0; s < 256; ++s)
        if (freq[s] > 0) {
            t.nodes.push_back({freq[s], s, -1, -1});
            pq.push({freq[s], static_cast<int>(t.nodes.size()) - 1});
        }
    t.distinct = t.nodes.size();
    if (pq.empty()) return t;

    // greedy step: merge the two lightest nodes until one tree is left
    while (pq.size() > 1) {
        Item a = pq.top(); pq.pop();
        Item b = pq.top(); pq.pop();
        t.nodes.push_back({a.first + b.first, -1, a.second, b.second});
        pq.push({a.first + b.first, static_cast<int>(t.nodes.size()) - 1});
    }
    t.root = pq.top().second;

    // read the codes off the tree: left edge = '0', right edge = '1'
    if (t.nodes[t.root].sym >= 0) {            // only one distinct symbol
        t.code[t.nodes[t.root].sym] = "0";
        return t;
    }
    std::vector<std::pair<int, std::string>> st;
    st.push_back({t.root, ""});
    while (!st.empty()) {
        std::pair<int, std::string> p = st.back();
        st.pop_back();
        const HNode& n = t.nodes[p.first];
        if (n.sym >= 0) t.code[n.sym] = p.second;
        else {
            st.push_back({n.left,  p.second + "0"});
            st.push_back({n.right, p.second + "1"});
        }
    }
    return t;
}

// Encode to a string of '0' / '1' characters. Bytes not in the table are skipped.
inline std::string huffmanEncode(const std::string& text, const HuffmanTable& t) {
    std::string bits;
    for (unsigned char c : text) bits += t.code[c];
    return bits;
}

// Decode. Returns false if `bits` has a character other than '0'/'1',
// or ends in the middle of a code.
inline bool huffmanDecode(const std::string& bits, const HuffmanTable& t, std::string& out) {
    out.clear();
    if (t.root < 0) return bits.empty();
    if (t.nodes[t.root].sym >= 0) {            // single-symbol text: every bit is that symbol
        for (char b : bits) {
            if (b != '0') return false;
            out += static_cast<char>(t.nodes[t.root].sym);
        }
        return true;
    }
    int cur = t.root;
    for (char b : bits) {
        if (b == '0')      cur = t.nodes[cur].left;
        else if (b == '1') cur = t.nodes[cur].right;
        else return false;
        if (t.nodes[cur].sym >= 0) {
            out += static_cast<char>(t.nodes[cur].sym);
            cur = t.root;
        }
    }
    return cur == t.root;
}

// Compressed size in bits, computed from the frequencies (no encoding needed).
inline unsigned long long huffmanBits(const HuffmanTable& t) {
    unsigned long long total = 0;
    for (std::size_t i = 0; i < t.distinct; ++i)
        total += t.nodes[i].w * t.code[t.nodes[i].sym].size();
    return total;
}

// Pack '0'/'1' characters into bytes (8 bits per byte, first bit = highest bit).
inline std::string packBits(const std::string& bits) {
    std::string out((bits.size() + 7) / 8, '\0');
    for (std::size_t i = 0; i < bits.size(); ++i)
        if (bits[i] == '1')
            out[i / 8] = static_cast<char>(out[i / 8] | (1 << (7 - i % 8)));
    return out;
}

inline std::string unpackBits(const std::string& bytes, std::size_t nbits) {
    if (nbits > bytes.size() * 8) nbits = bytes.size() * 8;
    std::string bits;
    bits.reserve(nbits);
    for (std::size_t i = 0; i < nbits; ++i)
        bits += ((static_cast<unsigned char>(bytes[i / 8]) >> (7 - i % 8)) & 1) ? '1' : '0';
    return bits;
}

// ---------- Event log built from a dataset ----------
inline std::string buildEventLog(const Dataset& d) {
    std::string log;
    for (const Reservation& r : d.reservations) {
        std::string id = std::to_string(r.id), st = std::to_string(r.start);
        log += "BOOKED id=" + id + " party=" + std::to_string(r.party) + " start=" + st + "\n";
        log += "SEATED id=" + id + " t=" + st + "\n";
    }
    for (const Order& o : d.orders)
        log += "ORDER_PLACED id=" + std::to_string(o.id) + " res=" + std::to_string(o.resId) +
               " t=" + std::to_string(o.placedAt) + "\n";
    return log;
}