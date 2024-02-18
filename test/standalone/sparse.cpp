#include <gtest/gtest.h>
#include "sparse.hpp"

template<typename FloatType, typename MatrixIndexType, typename ReturnIndexType>
void testSparse(void) {
/*
    A = [1  -1  -3  0    0; 
         0   5   4  6    0; 
         0   0  -4  2    7; 
         0   0   0  8    0; 
         0   0   0  0   -5];
*/

    utilities::Sparse<FloatType> A(5, 5);

    std::vector<FloatType> values = {1, -1, 5, -3, 4, -4, 6, 2, 8, 7, -5};
    std::vector<MatrixIndexType> iRow = {0, 0, 1, 0, 1, 2, 1, 2, 3, 2, 4};
    std::vector<MatrixIndexType> jCol = {0, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4};

    std::vector<ReturnIndexType> colBnd_ref = {0,1,3,6,9,11};
    std::vector<ReturnIndexType> iRow_ref = {0, 0, 1, 0, 1, 2, 1, 2, 3, 2, 4};

    A.set<MatrixIndexType>(iRow, jCol, values);

    std::vector<ReturnIndexType> colBnd(A.getNumberOfColumns() + 1, 0);
    std::vector<ReturnIndexType> iRow_out(A.getNumberOfNonZeroElements());
    std::vector<FloatType> val_out(A.getNumberOfNonZeroElements());

    A.getCsc<ReturnIndexType>(colBnd, iRow_out, val_out);

    for (std::size_t i = 0; i < A.getNumberOfColumns(); i++) {
        EXPECT_EQ(colBnd[i], colBnd_ref[i]);
        for (std::size_t j = colBnd[i]; j < colBnd[i+1]; j++) {
            EXPECT_EQ(iRow_out[j], iRow_ref[j]);
            EXPECT_EQ(val_out[j], values[j]);
        }
    }
}

// Demonstrate some basic assertions.
TEST(SparseTest, FloatTestSizeT)
{
    testSparse<float, std::size_t, std::size_t>();
}

TEST(SparseTest, DoubleTestSizeT)
{
    testSparse<double, std::size_t, std::size_t>();
}

TEST(SparseTest, FloatTestSizeTInt)
{
    testSparse<float, std::size_t, int>();
}

TEST(SparseTest, DoubleTestSizeTInt)
{
    testSparse<double, std::size_t, int>();
}

TEST(SparseTest, FloatTestInt)
{
    testSparse<float, int, int>();
}

TEST(SparseTest, DoubleTestInt)
{
    testSparse<double, int, int>();
}

TEST(SparseTest, FloatTestIntSizeT)
{
    testSparse<float, int, std::size_t>();
}

TEST(SparseTest, DoubleTestIntSizeT)
{
    testSparse<double, int, std::size_t>();
}
