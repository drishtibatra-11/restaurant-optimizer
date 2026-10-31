#include "csv_loader.hpp"
#include <cstdlib>
#include <fstream>
#include <set>

namespace {

struct Row { int line; std::vector<std::string> f; };

void trim(std::string& s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == ' ' || s.back() == '\t'))
        s.pop_back();
}

std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == sep) { out.push_back(cur); cur.clear(); }
        else cur += c;
    }
    out.push_back(cur);
    return out;
}

bool toInt(const std::string& s, int& v) {
    if (s.empty()) return false;
    char* end;
    long x = std::strtol(s.c_str(), &end, 10);
    if (*end != '\0') return false;
    v = static_cast<int>(x);
    return true;
}

bool toDouble(const std::string& s, double& v) {
    if (s.empty()) return false;
    char* end;
    v = std::strtod(s.c_str(), &end);
    return *end == '\0';
}

bool fail(std::string& err, const std::string& path, int line, const std::string& msg) {
    err = path + " line " + std::to_string(line) + ": " + msg;
    return false;
}

// Reads a CSV file, checks the header and column count.
bool readRows(const std::string& path, const std::string& header, size_t ncols,
              std::vector<Row>& rows, std::string& err) {
    std::ifstream in(path);
    if (!in) { err = "cannot open " + path; return false; }
    std::string line;
    if (!std::getline(in, line)) { err = path + ": file is empty"; return false; }
    trim(line);
    if (line != header) {
        err = path + " line 1: expected header '" + header + "'";
        return false;
    }
    int ln = 1;
    while (std::getline(in, line)) {
        ++ln;
        trim(line);
        if (line.empty()) continue;
        std::vector<std::string> f = split(line, ',');
        if (f.size() != ncols)
            return fail(err, path, ln, "expected " + std::to_string(ncols) + " columns");
        rows.push_back({ln, f});
    }
    if (rows.empty()) { err = path + ": no data rows"; return false; }
    return true;
}

bool ints(const Row& r, size_t n, int* out) {
    for (size_t i = 0; i < n; ++i)
        if (!toInt(r.f[i], out[i])) return false;
    return true;
}

const char* NUM = "not a valid number";

bool loadTables(const std::string& dir, Dataset& d, std::string& err) {
    std::string p = dir + "/tables.csv";
    std::vector<Row> rows;
    if (!readRows(p, "id,seats,zone", 3, rows, err)) return false;
    std::set<int> seen;
    for (const Row& r : rows) {
        int v[3];
        if (!ints(r, 3, v)) return fail(err, p, r.line, NUM);
        if (v[0] <= 0) return fail(err, p, r.line, "id must be positive");
        if (!seen.insert(v[0]).second) return fail(err, p, r.line, "duplicate id");
        if (v[1] <= 0) return fail(err, p, r.line, "seats must be positive");
        if (v[2] < 0) return fail(err, p, r.line, "zone must not be negative");
        d.tables.push_back({v[0], v[1], v[2]});
    }
    return true;
}

bool loadStations(const std::string& dir, Dataset& d, std::string& err) {
    std::string p = dir + "/stations.csv";
    std::vector<Row> rows;
    if (!readRows(p, "id,capacity", 2, rows, err)) return false;
    std::set<int> seen;
    for (const Row& r : rows) {
        int v[2];
        if (!ints(r, 2, v)) return fail(err, p, r.line, NUM);
        if (v[0] <= 0) return fail(err, p, r.line, "id must be positive");
        if (!seen.insert(v[0]).second) return fail(err, p, r.line, "duplicate id");
        if (v[1] <= 0) return fail(err, p, r.line, "capacity must be positive");
        d.stations.push_back({v[0], v[1]});
    }
    return true;
}

bool loadDishes(const std::string& dir, Dataset& d, const std::set<int>& stationIds,
                std::string& err) {
    std::string p = dir + "/dishes.csv";
    std::vector<Row> rows;
    if (!readRows(p, "id,prep_time,station,cost,profit", 5, rows, err)) return false;
    std::set<int> seen;
    for (const Row& r : rows) {
        int v[3];
        double cost, profit;
        if (!ints(r, 3, v) || !toDouble(r.f[3], cost) || !toDouble(r.f[4], profit))
            return fail(err, p, r.line, NUM);
        if (v[0] <= 0) return fail(err, p, r.line, "id must be positive");
        if (!seen.insert(v[0]).second) return fail(err, p, r.line, "duplicate id");
        if (v[1] <= 0) return fail(err, p, r.line, "prep_time must be positive");
        if (!stationIds.count(v[2])) return fail(err, p, r.line, "unknown station");
        if (cost < 0) return fail(err, p, r.line, "cost must not be negative");
        d.dishes.push_back({v[0], v[1], v[2], cost, profit});
    }
    return true;
}

bool loadReservations(const std::string& dir, Dataset& d, std::string& err) {
    std::string p = dir + "/reservations.csv";
    std::vector<Row> rows;
    if (!readRows(p, "id,party,start,dur,pref_zone", 5, rows, err)) return false;
    int maxSeats = 0;
    for (const Table& t : d.tables) if (t.seats > maxSeats) maxSeats = t.seats;
    std::set<int> seen;
    for (const Row& r : rows) {
        int v[5];
        if (!ints(r, 5, v)) return fail(err, p, r.line, NUM);
        if (v[0] <= 0) return fail(err, p, r.line, "id must be positive");
        if (!seen.insert(v[0]).second) return fail(err, p, r.line, "duplicate id");
        if (v[1] <= 0) return fail(err, p, r.line, "party must be positive");
        if (v[1] > maxSeats)
            return fail(err, p, r.line, "party larger than the biggest table");
        if (v[2] < 0 || v[2] > 1439) return fail(err, p, r.line, "start must be 0..1439");
        if (v[3] <= 0) return fail(err, p, r.line, "dur must be positive");
        if (v[4] < 0) return fail(err, p, r.line, "pref_zone must not be negative");
        d.reservations.push_back({v[0], v[1], v[2], v[3], v[4]});
    }
    return true;
}

bool loadOrders(const std::string& dir, Dataset& d, std::string& err) {
    std::string p = dir + "/orders.csv";
    std::vector<Row> rows;
    if (!readRows(p, "id,res_id,placed_at,dishes", 4, rows, err)) return false;
    std::set<int> resIds, dishIds, seen;
    for (const Reservation& r : d.reservations) resIds.insert(r.id);
    for (const Dish& x : d.dishes) dishIds.insert(x.id);
    for (const Row& r : rows) {
        int v[3];
        if (!ints(r, 3, v)) return fail(err, p, r.line, NUM);
        if (v[0] <= 0) return fail(err, p, r.line, "id must be positive");
        if (!seen.insert(v[0]).second) return fail(err, p, r.line, "duplicate id");
        if (!resIds.count(v[1])) return fail(err, p, r.line, "unknown reservation");
        if (v[2] < 0 || v[2] > 1439)
            return fail(err, p, r.line, "placed_at must be 0..1439");
        std::vector<int> list;
        for (const std::string& s : split(r.f[3], ';')) {
            int id;
            if (!toInt(s, id)) return fail(err, p, r.line, NUM);
            if (!dishIds.count(id)) return fail(err, p, r.line, "unknown dish");
            list.push_back(id);
        }
        d.orders.push_back({v[0], v[1], list, v[2]});
    }
    return true;
}

} // namespace

bool loadDataset(const std::string& dir, Dataset& d, std::string& err) {
    d = Dataset();
    if (!loadTables(dir, d, err)) return false;
    if (!loadStations(dir, d, err)) return false;
    std::set<int> stationIds;
    for (const Station& s : d.stations) stationIds.insert(s.id);
    if (!loadDishes(dir, d, stationIds, err)) return false;
    if (!loadReservations(dir, d, err)) return false;
    if (!loadOrders(dir, d, err)) return false;
    return true;
}