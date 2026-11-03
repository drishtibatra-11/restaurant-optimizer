#pragma once
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

// =====================================================================
// ResultTable: rows of named cells that can be exported as CSV or JSON.
// =====================================================================
inline std::string csvEscape(const std::string& s) {
    if (s.find_first_of(",\"\n\r") == std::string::npos) return s;
    std::string out = "\"";
    for (char c : s) { if (c == '"') out += '"'; out += c; }
    return out + "\"";
}

inline std::string jsonEscape(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    std::ostringstream h;
                    h << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                    out += h.str();
                } else out += c;
        }
    }
    return out;
}

// true for plain numbers like 12, -3, 4.50 (JSON allows these without quotes)
inline bool looksNumeric(const std::string& s) {
    std::size_t i = 0, n = s.size();
    if (i < n && s[i] == '-') ++i;
    std::size_t digits = 0;
    while (i < n && s[i] >= '0' && s[i] <= '9') { ++i; ++digits; }
    if (digits == 0) return false;
    if (digits > 1 && s[(s[0] == '-') ? 1 : 0] == '0') return false;      // no leading zeros
    if (i < n && s[i] == '.') {
        ++i;
        std::size_t frac = 0;
        while (i < n && s[i] >= '0' && s[i] <= '9') { ++i; ++frac; }
        if (frac == 0) return false;
    }
    return i == n;
}

inline std::string fmt(double v, int digits = 3) {
    std::ostringstream o;
    o << std::fixed << std::setprecision(digits) << v;
    return o.str();
}

class ResultTable {
public:
    void setColumns(const std::vector<std::string>& cols) { cols_ = cols; rows_.clear(); }
    // Returns false (and adds nothing) if the row has the wrong number of cells.
    bool addRow(const std::vector<std::string>& cells) {
        if (cells.size() != cols_.size()) return false;
        rows_.push_back(cells);
        return true;
    }
    std::size_t rows() const { return rows_.size(); }
    const std::vector<std::string>& columns() const { return cols_; }
    const std::vector<std::vector<std::string>>& data() const { return rows_; }
    void append(const ResultTable& o) {
        if (cols_.empty()) cols_ = o.cols_;
        if (o.cols_ == cols_) rows_.insert(rows_.end(), o.rows_.begin(), o.rows_.end());
    }

    std::string toCSV() const {
        std::string out;
        for (std::size_t i = 0; i < cols_.size(); ++i) out += (i ? "," : "") + csvEscape(cols_[i]);
        out += "\n";
        for (const auto& r : rows_) {
            for (std::size_t i = 0; i < r.size(); ++i) out += (i ? "," : "") + csvEscape(r[i]);
            out += "\n";
        }
        return out;
    }

    std::string toJSON() const {
        if (rows_.empty()) return "[]\n";
        std::string out = "[\n";
        for (std::size_t k = 0; k < rows_.size(); ++k) {
            out += "  {";
            for (std::size_t i = 0; i < cols_.size(); ++i) {
                out += (i ? ", " : "") + std::string("\"") + jsonEscape(cols_[i]) + "\": ";
                out += looksNumeric(rows_[k][i]) ? rows_[k][i] : "\"" + jsonEscape(rows_[k][i]) + "\"";
            }
            out += k + 1 < rows_.size() ? "},\n" : "}\n";
        }
        return out + "]\n";
    }

    // Writes CSV, or JSON if the path ends in ".json". Returns false if the file cannot be written.
    bool save(const std::string& path) const {
        std::ofstream f(path, std::ios::binary);
        if (!f) return false;
        bool json = path.size() >= 5 && path.compare(path.size() - 5, 5, ".json") == 0;
        f << (json ? toJSON() : toCSV());
        return static_cast<bool>(f);
    }

private:
    std::vector<std::string> cols_;
    std::vector<std::vector<std::string>> rows_;
};