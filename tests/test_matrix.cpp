#include "csv_loader.hpp"
#include "dc.hpp"
#include "test.hpp"
#include <random>
#include <string>
#include <vector>

static Matrix randomMatrix(int r, int c, std::mt19937& rng) {
    Matrix M(r, c);
    for (long long& x : M.a) x = static_cast<long long>(rng() % 19) - 9;   // -9..9
    return M;
}

static Matrix identity(int n) {
    Matrix I(n, n);
    for (int i = 0; i < n; ++i) I.at(i, i) = 1;
    return I;
}

int main() {
    std::mt19937 rng(5);

    // hand-calculated example: [[1,2],[3,4]] * [[5,6],[7,8]] = [[19,22],[43,50]]
    {
        Matrix A(2, 2), B(2, 2), E(2, 2);
        A.at(0, 0) = 1; A.at(0, 1) = 2; A.at(1, 0) = 3; A.at(1, 1) = 4;
        B.at(0, 0) = 5; B.at(0, 1) = 6; B.at(1, 0) = 7; B.at(1, 1) = 8;
        E.at(0, 0) = 19; E.at(0, 1) = 22; E.at(1, 0) = 43; E.at(1, 1) = 50;
        CHECK(matMulClassical(A, B) == E);
        CHECK(matMulStrassen(A, B, 1) == E);
    }

    // identity: A * I = A and I * A = A (6 is not a power of 2)
    {
        Matrix A = randomMatrix(6, 6, rng), I = identity(6);
        CHECK(matMulClassical(A, I) == A);
        CHECK(matMulStrassen(A, I, 1) == A);
        CHECK(matMulStrassen(I, A, 1) == A);
    }

    // square sizes that are powers of 2, with two different cutoffs
    {
        bool ok = true;
        for (int n : {1, 2, 4, 8, 16, 32}) {
            Matrix A = randomMatrix(n, n, rng), B = randomMatrix(n, n, rng);
            Matrix expect = matMulClassical(A, B);
            if (!(matMulStrassen(A, B, 1) == expect)) ok = false;
            if (!(matMulStrassen(A, B, 4) == expect)) ok = false;
        }
        CHECK(ok);
    }

    // sizes that are not powers of 2 (the code pads with zeros)
    {
        bool ok = true;
        for (int n : {3, 5, 7, 10, 17}) {
            Matrix A = randomMatrix(n, n, rng), B = randomMatrix(n, n, rng);
            if (!(matMulStrassen(A, B, 1) == matMulClassical(A, B))) ok = false;
        }
        CHECK(ok);
    }

    // rectangular matrices
    {
        bool ok = true;
        int shapes[][3] = {{3, 5, 2}, {7, 4, 9}, {1, 6, 1}, {5, 1, 5}};
        for (auto& s : shapes) {
            Matrix A = randomMatrix(s[0], s[1], rng), B = randomMatrix(s[1], s[2], rng);
            Matrix expect = matMulClassical(A, B);
            Matrix got = matMulStrassen(A, B, 1);
            if (!(got == expect) || got.rows != s[0] || got.cols != s[2]) ok = false;
        }
        CHECK(ok);
    }

    // sizes that do not match are rejected
    {
        Matrix A(2, 3), B(2, 3);
        CHECK(matMulClassical(A, B).rows == 0);
        CHECK(matMulStrassen(A, B).rows == 0);
    }

    // scalar multiplications: n^3 for classical, 7^log2(n) for Strassen (cutoff 1)
    {
        for (int n : {8, 16}) {
            Matrix A = randomMatrix(n, n, rng), B = randomMatrix(n, n, rng);
            long long c = 0, s = 0;
            matMulClassical(A, B, &c);
            matMulStrassen(A, B, 1, &s);
            if (n == 8)  { CHECK(c == 512);  CHECK(s == 343); }
            if (n == 16) { CHECK(c == 4096); CHECK(s == 2401); }
        }
    }

    // restaurant use: (hour x dish) demand times (dish x station) = hour x station load
    {
        Dataset d;
        std::string err;
        if (loadDataset("data/medium", d, err)) {
            int maxDish = 0, maxSt = 0;
            for (const Dish& x : d.dishes) maxDish = std::max(maxDish, x.id);
            for (const Station& s : d.stations) maxSt = std::max(maxSt, s.id);

            Matrix D(24, maxDish + 1), S(maxDish + 1, maxSt + 1);
            for (const Dish& x : d.dishes) S.at(x.id, x.station) = 1;
            long long items = 0;
            for (const Order& o : d.orders)
                for (int dishId : o.dishes) {
                    D.at(o.placedAt / 60, dishId) += 1;
                    ++items;
                }

            Matrix load = matMulClassical(D, S);
            CHECK(load == matMulStrassen(D, S, 1));
            long long total = 0;
            for (long long x : load.a) total += x;
            CHECK(total == items);                       // every dish lands on one station
            CHECK(load.rows == 24 && load.cols == maxSt + 1);
        } else {
            CHECK(false);  // run "make data" first
        }
    }

    return TEST_SUMMARY();
}