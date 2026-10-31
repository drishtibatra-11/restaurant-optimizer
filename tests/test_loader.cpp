#include "csv_loader.hpp"
#include "test.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static void put(const std::string& dir, const std::string& name, const std::string& text) {
    fs::create_directories(dir);
    std::ofstream f(dir + "/" + name);
    f << text;
}

static void makeValid(const std::string& dir) {
    put(dir, "tables.csv", "id,seats,zone\n1,4,1\n2,6,2\n");
    put(dir, "stations.csv", "id,capacity\n1,2\n");
    put(dir, "dishes.csv", "id,prep_time,station,cost,profit\n1,10,1,5,8\n2,15,1,6,9\n");
    put(dir, "reservations.csv", "id,party,start,dur,pref_zone\n1,4,1110,90,0\n2,2,1140,60,1\n");
    put(dir, "orders.csv", "id,res_id,placed_at,dishes\n1,1,1120,1;2\n2,2,1150,2\n");
}

// Makes a valid dataset, overwrites one file, expects a failure containing `needle`.
static bool rejects(const std::string& name, const std::string& file,
                    const std::string& text, const std::string& needle) {
    std::string dir = "build/t_" + name;
    fs::remove_all(dir);
    makeValid(dir);
    put(dir, file, text);
    Dataset d;
    std::string err;
    bool ok = loadDataset(dir, d, err);
    bool match = !ok && err.find(needle) != std::string::npos;
    if (!match) std::cerr << "  [" << name << "] got: " << err << "\n";
    return match;
}

int main() {
    // valid data loads correctly
    {
        std::string dir = "build/t_valid";
        fs::remove_all(dir);
        makeValid(dir);
        Dataset d;
        std::string err;
        CHECK(loadDataset(dir, d, err));
        CHECK(d.tables.size() == 2);
        CHECK(d.reservations.size() == 2);
        CHECK(d.reservations[0].end() == 1200);
        CHECK(d.orders[0].dishes.size() == 2);
    }
    // missing folder
    {
        Dataset d;
        std::string err;
        CHECK(!loadDataset("build/does_not_exist", d, err));
        CHECK(err.find("cannot open") != std::string::npos);
    }

    CHECK(rejects("negseats", "tables.csv", "id,seats,zone\n1,-4,1\n", "seats must be positive"));
    CHECK(rejects("empty", "tables.csv", "", "file is empty"));
    CHECK(rejects("headeronly", "tables.csv", "id,seats,zone\n", "no data rows"));
    CHECK(rejects("badheader", "tables.csv", "a,b,c\n1,4,1\n", "expected header"));
    CHECK(rejects("notnum", "tables.csv", "id,seats,zone\n1,abc,1\n", "not a valid number"));
    CHECK(rejects("dupid", "tables.csv", "id,seats,zone\n1,4,1\n1,2,1\n", "duplicate id"));
    CHECK(rejects("cols", "tables.csv", "id,seats,zone\n1,4\n", "columns"));
    CHECK(rejects("unkstation", "dishes.csv",
                  "id,prep_time,station,cost,profit\n1,10,9,5,8\n", "unknown station"));
    CHECK(rejects("bigparty", "reservations.csv",
                  "id,party,start,dur,pref_zone\n1,20,1110,90,0\n",
                  "party larger than the biggest table"));
    CHECK(rejects("badstart", "reservations.csv",
                  "id,party,start,dur,pref_zone\n1,2,2000,90,0\n", "start must be"));
    CHECK(rejects("unkres", "orders.csv",
                  "id,res_id,placed_at,dishes\n1,99,1120,1\n", "unknown reservation"));
    CHECK(rejects("unkdish", "orders.csv",
                  "id,res_id,placed_at,dishes\n1,1,1120,77\n", "unknown dish"));

    return TEST_SUMMARY();
}