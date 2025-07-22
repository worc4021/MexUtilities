#include "mex.hpp"
#include "mexAdapter.hpp"
#include "utilities.hpp"
#include <numeric>
#include <ranges>
#include <array>
#include <cassert>

void fill_range(std::ranges::range auto&& r, double value) {
    for (auto&& elem : r) {
        elem = value;
    }
}

namespace ranges {

auto row = [](const std::vector<std::size_t>& dims, std::size_t rowIndex) {
    assert(dims.size() >= 2 && "Dimensions must have at least 2 elements for row access");
    assert(rowIndex < dims[0] && "Row index out of bounds");
    return std::views::drop(rowIndex) | std::views::stride(dims[0]) | std::views::take(dims[1]);
};

auto page = [](const std::vector<std::size_t>& dims, std::size_t pageIndex) {
    assert(dims.size() >= 3 && "Dimensions must have at least 3 elements for page access");
    assert(pageIndex < dims[2] && "Page index out of bounds");
    return std::views::drop(pageIndex * dims[0] * dims[1]) | std::views::take(dims[0] * dims[1]);
};

auto col = [](const std::vector<std::size_t>& dims, std::size_t colIndex) {
    assert(dims.size() >= 2 && "Dimensions must have at least 2 elements for column access");
    assert(colIndex < dims[1] && "Column index out of bounds");
    return std::views::drop(colIndex * dims[0]) | std::views::take(dims[0]);
};

auto tensorial = [](const std::vector<std::size_t>& dims, std::size_t iRow, std::size_t jCol) {
    assert(dims.size() >= 3 && "Dimensions must have at least 3 elements for tensorial access");
    assert(iRow < dims[0] && "Row index out of bounds");
    return std::views::drop(iRow) | std::views::stride(dims[0]*dims[1]) | std::views::take(dims[2]);
};
} // namespace ranges

class MexFunction 
    : public matlab::mex::Function 
{

public:
    MexFunction()  {
            matlabPtr = getEngine();
    }
    ~MexFunction() = default;
    void operator()([[maybe_unused]]matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs) {
        if (inputs.size() < 1) {
            utilities::error("At least one input argument is required.");
        }

        matlab::data::TypedArray<double> x = std::move(inputs[0]);
        auto dims = x.getDimensions();

        matlab::data::ArrayFactory factory;

        std::vector<double> data(x.getNumberOfElements());
        
        fill_range(data | ranges::row(dims, 0), 1.0); // Initialize the data with zeros
        fill_range(data | ranges::col(dims, 3), 4.);
        fill_range(data | ranges::page(dims, 1) | ranges::row(dims, 2), 5.);
        fill_range(data | ranges::tensorial(dims, 1, 2), 6.);

        matlab::data::TypedArray<double> retval1 = factory.createArray<double>(dims);
        std::copy(data.begin(), data.end(), retval1.begin());

        outputs[0] = std::move(retval1);
    }
};