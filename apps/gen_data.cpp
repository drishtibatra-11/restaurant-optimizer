#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

static void usage() {
    std::cerr << "usage: gen_data <random|sorted|reversed|equal> <count> <folder>\n";
}

int main(int argc, char** argv) {
    if (argc != 4) { usage(); return 1; }
    std::string kind = argv[1];
    int n = std::atoi(argv[2]);
    std::string dir = argv[3];
    if (n <= 0 || (kind != "random" && kind != "sorted" &&
                   kind != "reversed" && kind != "equal")) {
        usage();
        return 1;
    }

    std::mt19937 rng(12345);
    auto rnd = [&](int lo, int hi) {
        return std::uniform_int_distribution<int>(lo, hi)(rng);
    };

    // tables: seats cycle 2,4,6,8 and zones 1..3
    {
        std::ofstream f(dir + "/tables.csv");
        if (!f) { std::cerr << "cannot write in " << dir << "\n"; return 1; }
        int nTables = std::max(5, n / 50);
        f << "id,seats,zone\n";
        for (int i = 1; i <= nTables; ++i)
            f << i << ',' << 2 * (1 + i % 4) << ',' << 1 + i % 3 << '\n';
    }
    // 5 kitchen stations
    {
        std::ofstream f(dir + "/stations.csv");
        f << "id,capacity\n";
        for (int i = 1; i <= 5; ++i) f << i << ',' << rnd(2, 4) << '\n';
    }
    // 20 dishes
    {
        std::ofstream f(dir + "/dishes.csv");
        f << "id,prep_time,station,cost,profit\n";
        for (int i = 1; i <= 20; ++i)
            f << i << ',' << rnd(5, 25) << ',' << 1 + i % 5 << ','
              << rnd(3, 15) << ',' << rnd(2, 12) << '\n';
    }
    // reservations: start times 11:00 to 23:00 in 5-minute steps
    std::vector<int> starts(n);
    for (int& s : starts) s = (kind == "equal") ? 1110 : 660 + 5 * rnd(0, 144);
    if (kind == "sorted") std::sort(starts.begin(), starts.end());
    if (kind == "reversed") std::sort(starts.rbegin(), starts.rend());
    {
        std::ofstream f(dir + "/reservations.csv");
        f << "id,party,start,dur,pref_zone\n";
        for (int i = 0; i < n; ++i) {
            bool eq = (kind == "equal");
            f << i + 1 << ',' << (eq ? 2 : rnd(1, 6)) << ',' << starts[i] << ','
              << (eq ? 90 : rnd(45, 120)) << ',' << (eq ? 0 : rnd(0, 3)) << '\n';
        }
    }
    // one order per reservation, 1 to 4 dishes each
    {
        std::ofstream f(dir + "/orders.csv");
        f << "id,res_id,placed_at,dishes\n";
        for (int i = 0; i < n; ++i) {
            f << i + 1 << ',' << i + 1 << ',' << starts[i] << ',';
            int k = rnd(1, 4);
            for (int j = 0; j < k; ++j) f << (j ? ";" : "") << rnd(1, 20);
            f << '\n';
        }
    }
    std::cout << "wrote " << n << " " << kind << " reservations to " << dir << "\n";
    return 0;
}