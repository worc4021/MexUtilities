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
        ts.a = 4;
        ts.b = 2;
        int result = ts.sum();
        utilities::printf("Sum of a and b: {}\n", result);
    }
};