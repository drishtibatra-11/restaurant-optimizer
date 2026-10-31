#include "model.hpp"
#include "test.hpp"
#include "timer.hpp"

int main() {
    Reservation r{1, 4, 1110, 90, 0};
    CHECK(r.end() == 1200);

    Dataset d;
    d.tables.push_back({1, 4, 1});
    d.tables.push_back({2, 2, 1});
    CHECK(d.tables.size() == 2);
    CHECK(d.reservations.empty());

    Timer t;
    CHECK(t.ms() >= 0.0);

    return TEST_SUMMARY();
}