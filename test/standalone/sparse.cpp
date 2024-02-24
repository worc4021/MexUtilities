#include <gtest/gtest.h>
#include "sparse.hpp"

template<typename FloatType, typename MatrixIndexType, typename ReturnIndexType>
void testSparseCSC(void) {
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

template<typename FloatType, typename MatrixIndexType, typename ReturnIndexType>
void testSparseCSR(void) {
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

    std::vector<ReturnIndexType> rowBnd_ref = {0,3,6,9,10,11};
    std::vector<ReturnIndexType> jCol_ref = {0,1,2,1,2,3,2,3,4,3,4};
    std::vector<FloatType> val_ref = {1,-1,-3,5,4,6,-4,2,7,8,-5};

    A.set<MatrixIndexType>(iRow, jCol, values);

    std::vector<ReturnIndexType> rowBnd(A.getNumberOfRows() + 1, 0);
    std::vector<ReturnIndexType> jCol_out(A.getNumberOfNonZeroElements());
    std::vector<FloatType> val_out(A.getNumberOfNonZeroElements());
    
    A.getCsr<ReturnIndexType>(rowBnd, jCol_out, val_out);    

    for (std::size_t i = 0; i < A.getNumberOfRows(); i++) {
        EXPECT_EQ(rowBnd[i], rowBnd_ref[i]);
        for (std::size_t j = rowBnd[i]; j < rowBnd[i+1]; j++) {
            EXPECT_EQ(jCol_out[j], jCol_ref[j]);
            EXPECT_EQ(val_out[j], val_ref[j]);
        }
    }
}

// Demonstrate some basic assertions.
TEST(SparseTest, CSC_FloatTestSizeT)
{
    testSparseCSC<float, std::size_t, std::size_t>();
}

TEST(SparseTest, CSC_DoubleTestSizeT)
{
    testSparseCSC<double, std::size_t, std::size_t>();
}

TEST(SparseTest, CSC_FloatTestSizeTInt)
{
    testSparseCSC<float, std::size_t, int>();
}

TEST(SparseTest, CSC_DoubleTestSizeTInt)
{
    testSparseCSC<double, std::size_t, int>();
}

TEST(SparseTest, CSC_FloatTestInt)
{
    testSparseCSC<float, int, int>();
}

TEST(SparseTest, CSC_DoubleTestInt)
{
    testSparseCSC<double, int, int>();
}

TEST(SparseTest, CSC_FloatTestIntSizeT)
{
    testSparseCSC<float, int, std::size_t>();
}

TEST(SparseTest, CSC_DoubleTestIntSizeT)
{
    testSparseCSC<double, int, std::size_t>();
}

TEST(SparseTest, CSR_FloatTestSizeT)
{
    testSparseCSR<float, std::size_t, std::size_t>();
}

TEST(SparseTest, CSR_DoubleTestSizeT)
{
    testSparseCSR<double, std::size_t, std::size_t>();
}

TEST(SparseTest, CSR_FloatTestSizeTInt)
{
    testSparseCSR<float, std::size_t, int>();
}

TEST(SparseTest, CSR_DoubleTestSizeTInt)
{
    testSparseCSR<double, std::size_t, int>();
}

TEST(SparseTest, CSR_FloatTestInt)
{
    testSparseCSR<float, int, int>();
}

TEST(SparseTest, CSR_DoubleTestInt)
{
    testSparseCSR<double, int, int>();
}

TEST(SparseTest, CSR_FloatTestIntSizeT)
{
    testSparseCSR<float, int, std::size_t>();
}

TEST(SparseTest, CSR_DoubleTestIntSizeT)
{
    testSparseCSR<double, int, std::size_t>();
}
