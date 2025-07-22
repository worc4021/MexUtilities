#include <vector>
#include <ranges>
#include <fmt/ranges.h>


void print_range(const std::ranges::range auto& r) {
    fmt::print("{}\n", r);
}

void increment_range(std::ranges::range auto&& r) {
    for (auto& elem : r) {
        elem += 5;
    }
}


int main() {
    // std::vector<int> v {0,1,2,3,4,5,6,7,8};
    // fmt::print("{}\n", std::views::stride(v,2)); // [0,2,4,6,8]
    // fmt::print("{}\n", std::views::stride(v,3)); // [0,3,6]
    // // using pipeline notation:
    // fmt::print("{}\n", v | std::views::drop(3) | std::views::stride(2)); // [0,2,4,6,8]
    // // using an explicit view object:
    // std::ranges::stride_view sv2 {v,2};
    // fmt::print("{}\n", sv2); // [0,2,4,6,8]
    // increment_range(v | std::views::drop(3) | std::views::stride(2));
    // print_range(v | std::views::drop(3) | std::views::stride(2));
    // print_range(v | std::views::all );

    constexpr auto nRows = 3;
    constexpr auto nCols = 4;
    constexpr auto nPages = 2;
    std::vector<int> v(nRows * nCols * nPages);

    for (std::size_t iPage = 0; iPage < nPages; ++iPage) {
        for (std::size_t jCol = 0; jCol < nCols; ++jCol) {
            for (std::size_t iRow = 0; iRow < nRows; ++iRow) {
                v[iPage * nRows * nCols + jCol * nRows + iRow] = iPage * 100 + jCol * 10 + iRow;
            }
        }
    }

    auto page1 = v | std::views::take(nRows * nCols);
    auto page2 = v | std::views::drop(nRows * nCols) | std::views::take(nRows * nCols);
    auto page2row2 = page2 | std::views::drop(1) | std::views::stride(nRows) | std::views::take(nCols);
    print_range(page1);
    print_range(page2);
    print_range(page2row2);

    return 0;
}