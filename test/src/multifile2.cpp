#include "mex.hpp"
#include "mexAdapter.hpp"
#include "multifile.hpp"
#include "utilities.hpp"

class MexFunction 
    : public matlab::mex::Function 
{

public:
    MexFunction()  {
            matlabPtr = getEngine();
    }
    ~MexFunction() = default;
    void operator()([[maybe_unused]]matlab::mex::ArgumentList outputs, [[maybe_unused]]matlab::mex::ArgumentList inputs) {
        TestStruct ts;
        if (inputs.size() == 2){
            matlab::data::TypedArray<double> input1 = std::move(inputs[0]);
            matlab::data::TypedArray<double> input2 = std::move(inputs[1]);
            ts.a = static_cast<int>(input1[0]);
            ts.b = static_cast<int>(input2[0]);
        }else{
            ts.a = 1;
            ts.b = 2;
        }
        int result = ts.sum();
        if (outputs.size() > 0){
            matlab::data::ArrayFactory factory;
            outputs[0] = factory.createScalar(result);
        } else {
            utilities::printf("Sum of a and b: {}\n", result);
        }
    }
};