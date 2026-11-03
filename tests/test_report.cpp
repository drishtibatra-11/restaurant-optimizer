#include "report.hpp"
#include "test.hpp"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

static std::string slurp(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

int main() {
    // escaping
    CHECK(csvEscape("plain") == "plain");
    CHECK(csvEscape("a,b") == "\"a,b\"");
    CHECK(csvEscape("say \"hi\"") == "\"say \"\"hi\"\"\"");
    CHECK(csvEscape("two\nlines") == "\"two\nlines\"");
    CHECK(jsonEscape("a\"b\\c\n\t") == "a\\\"b\\\\c\\n\\t");
    CHECK(jsonEscape(std::string(1, '\x01')) == "\\u0001");

    // numbers are recognised (JSON must not quote them, and must not accept things like 007 or 1.)
    CHECK(looksNumeric("0") && looksNumeric("12") && looksNumeric("-3") && looksNumeric("4.50"));
    CHECK(!looksNumeric("") && !looksNumeric("-") && !looksNumeric("007") && !looksNumeric("1."));
    CHECK(!looksNumeric("1e5") && !looksNumeric("abc") && !looksNumeric("1 2") && !looksNumeric(".5"));
    CHECK(fmt(2.0, 1) == "2.0" && fmt(0.12345, 2) == "0.12");

    // an empty table
    {
        ResultTable t;
        CHECK(t.toJSON() == "[]\n");
        t.setColumns({"a", "b"});
        CHECK(t.toCSV() == "a,b\n" && t.toJSON() == "[]\n" && t.rows() == 0);
    }

    // rows with the wrong number of cells are refused
    {
        ResultTable t;
        t.setColumns({"name", "value"});
        CHECK(!t.addRow({"only one"}) && !t.addRow({"a", "b", "c"}) && t.rows() == 0);
        CHECK(t.addRow({"merge sort", "12.5"}) && t.addRow({"say \"hi\", ok", "n/a"}));
        CHECK(t.rows() == 2);
        CHECK(t.toCSV() == "name,value\nmerge sort,12.5\n\"say \"\"hi\"\", ok\",n/a\n");
        CHECK(t.toJSON() ==
              "[\n  {\"name\": \"merge sort\", \"value\": 12.5},\n"
              "  {\"name\": \"say \\\"hi\\\", ok\", \"value\": \"n/a\"}\n]\n");
    }

    // appending tables with the same columns, and refusing different ones
    {
        ResultTable a, b, c;
        a.setColumns({"x"});
        a.addRow({"1"});
        b.setColumns({"x"});
        b.addRow({"2"});
        c.setColumns({"y"});
        c.addRow({"3"});
        a.append(b);
        a.append(c);
        CHECK(a.rows() == 2);
        ResultTable empty;
        empty.append(b);
        CHECK(empty.rows() == 1 && empty.columns() == b.columns());
    }

    // saving: .json gives JSON, anything else gives CSV, and a bad path fails
    {
        ResultTable t;
        t.setColumns({"k", "v"});
        t.addRow({"a", "1"});
        CHECK(t.save("build/test_report_out.csv") && slurp("build/test_report_out.csv") == "k,v\na,1\n");
        CHECK(t.save("build/test_report_out.json") &&
              slurp("build/test_report_out.json") == "[\n  {\"k\": \"a\", \"v\": 1}\n]\n");
        CHECK(!t.save("build/no_such_folder/x.csv"));
        std::remove("build/test_report_out.csv");
        std::remove("build/test_report_out.json");
    }

    return TEST_SUMMARY();
}