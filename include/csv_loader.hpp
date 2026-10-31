#pragma once
#include "model.hpp"
#include <string>

// Loads and validates the 5 CSV files in `dir`.
// Returns true on success. On failure returns false and puts a
// clear message (file, line, reason) in `err`.
bool loadDataset(const std::string& dir, Dataset& d, std::string& err);