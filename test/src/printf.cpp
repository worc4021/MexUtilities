#include "mex.hpp"
#include "mexAdapter.hpp"
#include "utilities.hpp"

class MexFunction 
    : public matlab::mex::Function 
{

public:
    MexFunction()  {
            matlabPtr = getEngine();
    }
    ~MexFunction() = default;
    void operator()([[maybe_unused]]matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs) {
        if (inputs.size()) {
            if (utilities::isnumeric(inputs[0])) {
                matlab::data::TypedArray<double> x = std::move(inputs[0]);
                utilities::printf("x: {}\n", x);
            }
        } else {
            std::vector<int> x = {1, 2, 3};
            utilities::printf("x: {}\n", x);
        }
    }
};