// Runs the demonstration scenarios from the command line.
//   simulator.exe <1|2|3|all> [--data <folder>] [--csv <file>] [--json <file>]
#include "report.hpp"
#include "scenarios.hpp"
#include <cstdlib>
#include <iostream>
#include <string>

static void usage() {
    std::cerr << "usage: simulator <1|2|3|all> [--data <folder>] [--csv <file>] [--json <file>]\n"
                 "  1 = normal evening, 2 = capacity shock, 3 = large-data stress, all = run all three\n";
}

int main(int argc, char** argv) {
    if (argc < 2) { usage(); return 1; }
    std::string which = argv[1], data, csv, json;
    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];
        if ((a == "--data" || a == "--csv" || a == "--json") && i + 1 < argc) {
            std::string v = argv[++i];
            (a == "--data" ? data : a == "--csv" ? csv : json) = v;
        } else { usage(); return 1; }
    }
    std::vector<int> list;
    if (which == "all") list = {1, 2, 3};
    else if (which == "1" || which == "2" || which == "3") list = {std::atoi(which.c_str())};
    else { usage(); return 1; }

    ResultTable all;
    bool failed = false;
    for (int s : list) {
        ResultTable t = runScenario(s, data.empty() ? defaultScenarioData(s) : data, std::cout);
        if (t.rows() == 0) failed = true;
        all.append(t);
    }
    if (!csv.empty()) {
        if (all.save(csv)) std::cout << "\nWrote " << csv << "\n";
        else { std::cerr << "cannot write " << csv << "\n"; failed = true; }
    }
    if (!json.empty()) {
        if (all.save(json)) std::cout << "\nWrote " << json << "\n";
        else { std::cerr << "cannot write " << json << "\n"; failed = true; }
    }
    return failed ? 1 : 0;
}