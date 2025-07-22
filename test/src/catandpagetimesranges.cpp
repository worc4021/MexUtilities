#include "mex.hpp"
#include "mexAdapter.hpp"
#include "details/blockdata.hpp"
#include "utilities.hpp"
#include "blas.h"
#include <numeric>
#include <ranges>
#include <algorithm>
#include <array>
#include <cassert>


struct InputHelper {
    matlab::data::ArrayFactory factory;
    matlab::data::StructArray toplevel;
    matlab::data::StructArray level1;
    InputHelper(matlab::data::StructArray &&inp)
        : toplevel{(matlab::data::StructArray &&)std::move(inp)}
        , level1{(matlab::data::StructArray &&)std::move(toplevel[0]["level1"])} {}
    
    template<std::size_t N>
    std::size_t getNumberInDimension() {
        std::size_t n{ 1 };
        auto dims = toplevel[0]["x"].getDimensions();
        std::size_t k = (dims.size() > N ? dims[N] : 1);
        n = (k > n) ? k : n;
        dims = level1[0]["x"].getDimensions();
        k = (dims.size() > N ? dims[N] : 1);
        if (k != n && n != 1 && k != 1) {
            utilities::error("Input arrays must have the same dimension in dimension {}.\n",N);
        }
        n = (k > n) ? k : n;
        return n;
    }


    std::size_t getNumberOfPages() {
        return getNumberInDimension<2>();
    }

    std::size_t getNumberOfPoints() {
        return getNumberInDimension<1>();
    }    
};

struct OutputHelper {
    matlab::data::ArrayFactory factory;
    matlab::data::StructArray toplevel;
    matlab::data::StructArray level2;
    OutputHelper()
        : toplevel{factory.createStructArray({1,1},{"level2","y"})}
        , level2{factory.createStructArray({1,1},{"y"})} 
        {}
    matlab::data::StructArray getNested() {
        toplevel[0]["level2"] = std::move(level2);
        return std::move(toplevel);
    }
};

struct ChainRule {
    std::size_t nInputs{2};
    std::size_t nOutputs{2};
    utilities::details::BlockDataV<2, double> inputs_x;
    utilities::details::BlockDataV<3, double> inputs_J;
    utilities::details::BlockDataV<2, double> outputs_x;
    utilities::details::BlockDataV<3, double> outputs_J;

    InputHelper inputHelper;
    OutputHelper outputHelper;
    ChainRule(matlab::data::StructArray &&inp)
        : inputHelper{std::move(inp)}
        , outputHelper{} 
    {
        std::size_t nPoints = inputHelper.getNumberOfPoints();
        std::size_t nDirections = inputHelper.getNumberOfPages();
        inputs_x.resize(nInputs, nPoints);
        inputs_J.resize(nInputs, nDirections, nPoints);
        outputs_x.resize(nOutputs, nPoints);
        outputs_J.resize(nOutputs, nDirections, nPoints);
    }

    void mapSingleInput(matlab::data::Array &&x, std::size_t inputIndex) {
        std::size_t nPoints{ getNumberOfPoints()};
        std::size_t nPages{ getNumberOfDirections() };

        auto dims = x.getDimensions();
        if (x.getType() == matlab::data::ArrayType::DOUBLE) {
            matlab::data::TypedArray<double> xref = std::move(x);
            auto targetRow = inputs_x.all() | inputs_x.row(inputIndex);
            if (dims[0] > 1)
                utilities::error("Only scalar inputs supported");
            if (dims[1] == 1) 
                std::ranges::fill(std::ranges::begin(targetRow), std::ranges::end(targetRow), xref[0]);
            else {
                std::copy(xref.begin(), xref.end(), std::ranges::begin(targetRow));
            }
                
            
        } else if (x.getType() == matlab::data::ArrayType::COMPLEX_DOUBLE) {
            utilities::details::BlockDataV<3, std::complex<double>> input_bd(std::move(x));
            std::size_t nDir = dims.size() > 2 ? dims[2] : 1;
            auto inputRow = input_bd.all() | input_bd.row(0);
            auto targetRow = inputs_x.all() | inputs_x.row(inputIndex);
            std::transform( std::ranges::begin(inputRow), 
                            std::ranges::end(inputRow), 
                            std::ranges::begin(targetRow),
                            [](const std::complex<double>& val) { return val.real(); });
            
            for (std::size_t iDir = 0; iDir < nDir; ++iDir) {
                auto inputRow = input_bd.all() | input_bd.page(iDir) | input_bd.row(0);
                auto targetRowJ = inputs_J.all() | inputs_J.tensorial(inputIndex, iDir);
                std::transform( std::ranges::begin(inputRow), 
                                std::ranges::end(inputRow), 
                                std::ranges::begin(targetRowJ), 
                                [](const std::complex<double>& val) { return val.imag() * 1e100; });
            }
            
        } else {
            utilities::error("Unsupported data type for input {}. Expected double or single.", inputIndex + 1);
        }
    }

    std::size_t getNumberOfPoints() const {
        return inputs_x.size() / nInputs ;
    }

    std::size_t getNumberOfDirections() const {
        return inputs_J.size() / (nInputs * getNumberOfPoints());
    }
 
    void initialise() {
        mapSingleInput((matlab::data::Array &&)std::move(inputHelper.toplevel[0]["x"]), 0);
        mapSingleInput((matlab::data::Array &&)std::move(inputHelper.level1[0]["x"]), 1);
    }

    void singleModelCallMockup(std::span<double> y, std::span<double> J, std::size_t iPoint) {
        std::size_t nPoints{ getNumberOfPoints() };
        std::size_t nPages{ getNumberOfDirections() };

        const char transa = 'N';
        const char transb = 'N';
        ptrdiff_t m = nOutputs;
        ptrdiff_t k = nPages;
        ptrdiff_t n = nInputs;
        ptrdiff_t lda = nOutputs;
        ptrdiff_t ldb = nInputs;
        double alpha = 1.0;
        double beta = 0.0;
        
        auto activeInputPage = inputs_J.all() | inputs_J.page(iPoint);
        auto activeOutputPage = outputs_J.all() | outputs_J.page(iPoint);
        
        dgemm(&transa, &transb, &m, &n, &k, &alpha, 
                J.data(), &lda, std::ranges::data(activeInputPage), &ldb, &beta,
                std::ranges::data(activeOutputPage), &lda);

        auto outputCol = outputs_x.all() | outputs_x.col(iPoint);
        std::copy_n(y.begin(), nOutputs, std::ranges::begin(outputCol));
        
    }

    matlab::data::TypedArray<std::complex<double>> outputByIndex(std::size_t idx) {
        matlab::data::ArrayFactory f;
		std::size_t nPoints = getNumberOfPoints();
        std::size_t nDirections = getNumberOfDirections();
        
        matlab::data::buffer_ptr_t<std::complex<double>> retval = f.createBuffer<std::complex<double>>(getNumberOfPoints() * getNumberOfDirections());
        std::span<std::complex<double>> retval_span(retval.get(), nPoints * nDirections);
        auto outputRow = outputs_x.all() | outputs_x.row(idx);
        for (std::size_t iDirection = 0; iDirection < nDirections; ++iDirection) {
            auto outputJac = outputs_J.all() | outputs_J.tensorial(idx, iDirection);
            auto targetRow = retval_span | std::views::drop(iDirection * nPoints) | std::views::take(nPoints);
            std::transform( std::ranges::begin(outputRow), 
                            std::ranges::end(outputRow), 
                            std::ranges::begin(outputJac), 
                            std::ranges::begin(targetRow), 
                            [](const double& val, const double& j_val) {
                                return std::complex<double>(val, j_val * 1e-100);
                            });
		}
		return f.createArrayFromBuffer<std::complex<double>>({ 1, nPoints, nDirections }, std::move(retval));
    }

    matlab::data::StructArray getNested() {
        outputHelper.toplevel[0]["y"] = outputByIndex(0);
        outputHelper.level2[0]["y"] = outputByIndex(1);
        return outputHelper.getNested();
    }
};


class MexFunction 
    : public matlab::mex::Function 
{

public:
    MexFunction()  {
            matlabPtr = getEngine();
    }
    ~MexFunction() = default;
    void operator()([[maybe_unused]]matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs) {
        
        if (inputs.size() != 3) {
            utilities::error("Pass input struct, yref, J\n");
        }

        matlab::data::ArrayFactory factory;
        matlab::data::StructArray inputStruct = inputs[0];
        
        InputHelper inputHelper(std::move(inputStruct));
        OutputHelper outputHelper;
        ChainRule chainRule(std::move(inputHelper.toplevel));
        chainRule.initialise();

        std::size_t nPoints = chainRule.getNumberOfPoints();

        matlab::data::TypedArray<double> yref = std::move(inputs[1]);
        matlab::data::TypedArray<double> J = std::move(inputs[2]);
        matlab::data::buffer_ptr_t<double> y_ptr = yref.release();
        matlab::data::buffer_ptr_t<double> J_ptr = J.release();

         for (std::size_t iPoint = 0; iPoint < nPoints; ++iPoint) {
             // Map the outputs
             chainRule.singleModelCallMockup(
                 std::span<double>(y_ptr.get(), 2),
                 std::span<double>(J_ptr.get(), 4),
                 iPoint);
         }


         outputs[0] = chainRule.getNested();
    }
};