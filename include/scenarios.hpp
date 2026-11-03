#pragma once
#include "report.hpp"
#include <iosfwd>
#include <string>

// The three demonstration scenarios.
//   1 = normal evening          (default data: data/medium)
//   2 = capacity or constraint shock (default data: data/medium)
//   3 = large-data stress case  (default data: data/large)
// Each one runs at least two strategies on the SAME input and prints a readable report to `out`.
// The same numbers are returned as a table with the columns:
//   scenario, section, strategy, metric, value
// If the dataset cannot be loaded, the error is printed and the table is empty.
ResultTable runScenario(int which, const std::string& dataDir, std::ostream& out);

// The default dataset folder for a scenario number.
std::string defaultScenarioData(int which);