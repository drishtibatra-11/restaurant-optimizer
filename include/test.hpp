#pragma once
#include <iostream>

namespace testing {
inline int& checks()   { static int n = 0; return n; }
inline int& failures() { static int n = 0; return n; }
inline int summary() {
    std::cout << checks() - failures() << "/" << checks() << " checks passed\n";
    return failures() == 0 ? 0 : 1;
}
}

#define CHECK(cond)                                                      \
    do {                                                                 \
        ++testing::checks();                                             \
        if (!(cond)) {                                                   \
            ++testing::failures();                                       \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__          \
                      << "  " << #cond << "\n";                          \
        }                                                                \
    } while (0)

#define TEST_SUMMARY() testing::summary()