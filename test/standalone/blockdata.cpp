#include <gtest/gtest.h>
#include "details/blockdata.hpp"

TEST(BlockDataTest, SingleDimension)
{
    utilities::details::BlockData<1, double> bd(10);
    EXPECT_EQ(bd.size(), 10);
    for (std::size_t i = 0; i < 10; ++i)
    {
        bd[i] = static_cast<double>(i);
    }
    for (std::size_t i = 0; i < 10; ++i)
    {
        EXPECT_EQ(bd[i], static_cast<double>(i));
    }
}

TEST(BlockDataTest, Dimensions)
{
    constexpr auto nRows = 3;
    constexpr auto nCols = 4;
    constexpr auto nPages = 2;
    utilities::details::BlockData<2, double> bd(nRows, nCols);

    EXPECT_EQ(bd.size(), nRows * nCols);
    EXPECT_EQ(bd.nRows(), nRows);
    EXPECT_EQ(bd.nCols(), nCols);

    utilities::details::BlockData<3, double> bd3(nRows, nCols, nPages);
    EXPECT_EQ(bd3.size(), nRows * nCols * nPages);
    EXPECT_EQ(bd3.nRows(), nRows);
    EXPECT_EQ(bd3.nCols(), nCols);
    EXPECT_EQ(bd3.nPages(), nPages);
}

TEST(BlockDataTest, Transpose)
{
    constexpr auto nRows = 3;
    constexpr auto nCols = 4;
    constexpr auto nPages = 7; // For 2D transpose, we can ignore pages
    utilities::details::BlockData<2, double> bd(nRows, nCols);
    double offset = 0.;
    for (std::size_t iCol = 0; iCol < nCols; ++iCol)
    {
        std::transform(bd.column(iCol).begin(), bd.column(iCol).end(), bd.column(iCol).begin(), [&offset](double &)
                       { return offset++; });
    }

    for (auto iRow = 0.; iRow < nRows; ++iRow)
    {
        for (auto iCol = 0.; iCol < nCols; ++iCol)
        {
            EXPECT_EQ(bd(iRow, iCol), iRow + iCol * nRows);
        }
    }

    bd.transpose();

    EXPECT_EQ(bd.nRows(), nCols);
    EXPECT_EQ(bd.nCols(), nRows);

    for (auto iCol = 0; iCol < nRows; ++iCol)
    {
        for (auto iRow = 0; iRow < nCols; ++iRow)
        {
            EXPECT_EQ(bd(iRow, iCol), iCol + iRow * nRows) << "Error in index iRow = " << iRow << ", jCol = " << iCol;
        }
    }

    // Finish the transposing and permuting of dimensions for tensors.
    utilities::details::BlockData<3, double> bd3(nRows, nCols, nPages);
    offset = 0.;
    for (std::size_t kPage = 0; kPage < nPages; ++kPage)
    {
        for (std::size_t iCol = 0; iCol < nCols; ++iCol)
        {
            std::transform(bd3.page(kPage).column(iCol).begin(), bd3.page(kPage).column(iCol).end(), bd3.page(kPage).column(iCol).begin(), [&offset](double &)
                           { return offset++; });
        }
    }
    
}

template<typename T>
void my_fill(std::span<T> a, T value)
{
    std::fill(a.begin(), a.end(), value);
}

TEST(BlockDataTest, Span) {
    constexpr auto nRows = 3;
    constexpr auto nCols = 4;
    utilities::details::BlockData<2, double> bd(nRows, nCols);
    my_fill<double>(bd.column(0), 1.0);
    my_fill<double>(bd.column(1), 2.0);
    my_fill<double>(bd.column(2), 3.0);
    my_fill<double>(bd.column(3), 4.0);
    for (std::size_t iRow = 0; iRow < nRows; ++iRow)
    {
        for (std::size_t iCol = 0; iCol < nCols; ++iCol)
        {
            EXPECT_EQ(bd(iRow, iCol), static_cast<double>(iCol + 1));
        }
    }

    utilities::details::BlockData<2, double> bd2(nRows, nCols);

    my_fill<double>(bd2.row(0), 5.0);
    my_fill<double>(bd2.row(1), 6.0);
    my_fill<double>(bd2.row(2), 7.0);
    for (std::size_t iRow = 0; iRow < nRows; ++iRow)
    {
        for (std::size_t iCol = 0; iCol < nCols; ++iCol)
        {
            EXPECT_EQ(bd2(iRow, iCol), static_cast<double>(iRow + 5));
        }
    }

}