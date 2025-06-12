#include "mex.hpp"
#include "mexAdapter.hpp"
#include "details/blockdata.hpp"
#include "utilities.hpp"
#include "blas.h"
#include <span>


// Strided output iterator adapter for column-major row
template<typename T>
class strided_row_iterator {
    T* base;
    std::size_t stride;
    std::size_t index;
public:
    using iterator_category = std::output_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    strided_row_iterator(T* base, std::size_t stride, std::size_t index=0)
        : base(base), stride(stride), index(index) {}

    strided_row_iterator& operator++() { ++index; return *this; }
    strided_row_iterator operator++(int) { auto tmp = *this; ++*this; return tmp; }
    T& operator*() { return base[index * stride]; }
};

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
    std::size_t nInputs{3};
    std::size_t nOutputs{2};
    utilities::details::BlockData<2, double> inputs_x;
    utilities::details::BlockData<3, double> inputs_J;
    utilities::details::BlockData<2, double> outputs_x;
    utilities::details::BlockData<3, double> outputs_J;

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
            if (dims[1] == 1) {
                for (std::size_t iDim = 0; iDim < dims[0]; ++iDim) {
                    std::fill(inputs_x.row(inputIndex).begin(), inputs_x.row(inputIndex).end(), xref[iDim]);
                }                
            } else {
                utilities::details::BlockData<2, double> input_bd(std::move(x));
                for (std::size_t iDim = 0; iDim < dims[0]; ++iDim) {
                    std::copy(input_bd.row(iDim).begin(), input_bd.row(iDim).end(), inputs_x.row(inputIndex+iDim).begin());
                }
            }
        } else if (x.getType() == matlab::data::ArrayType::COMPLEX_DOUBLE) {
            utilities::details::BlockData<3, std::complex<double>> input_bd(std::move(x));
            std::size_t nDir = dims.size() > 2 ? dims[2] : 1;
            for (std::size_t iRow = 0; iRow < dims[0]; ++iRow) {
                std::transform(input_bd.row(iRow).begin(), input_bd.row(iRow).end(), inputs_x.row(inputIndex + iRow).begin(), [](const std::complex<double>& val) { return val.real(); });
                for (std::size_t iDir = 0; iDir < nDir; ++iDir) {
                    std::transform(input_bd.page(iDir).row(iRow).begin(), input_bd.page(iDir).row(iRow).end(), inputs_J.tensorial(inputIndex + iRow, iDir).begin(), [](const std::complex<double>& val) { return val.imag() * 1e100; });
                }
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
        
        dgemm(&transa, &transb, &m, &n, &k, &alpha, 
                J.data(), &lda, inputs_J.page(iPoint).data(), &ldb, &beta,
                outputs_J.page(iPoint).data(), &lda);

        std::copy_n(y.begin(), nOutputs, outputs_x.column(iPoint).begin());
        
    }

    matlab::data::TypedArray<std::complex<double>> outputByIndex(std::size_t idx) {
        matlab::data::ArrayFactory f;
		std::size_t nPoints = getNumberOfPoints();
        std::size_t nDirections = getNumberOfDirections();
        
        matlab::data::buffer_ptr_t<std::complex<double>> retval = f.createBuffer<std::complex<double>>(getNumberOfPoints() * getNumberOfDirections());
        for (std::size_t iDirection = 0; iDirection < nDirections; ++iDirection) {
            std::transform(outputs_x.row(idx).begin(), outputs_x.row(idx).end(), outputs_J.tensorial(idx, iDirection).begin(), retval.get() + iDirection * nPoints, 
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
                 std::span<double>(J_ptr.get(), 6),
                 iPoint);
         }

         outputs[0] = chainRule.getNested();
    }
};