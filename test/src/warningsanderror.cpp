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
            std::string cmd = utilities::getstringvalue(inputs[0]);
            if ("warn" == cmd) {
                utilities::warning("This is an unspecific warning message");
            } else if ("wspec" == cmd) {
                utilities::warnWithId("specific", "This is a specific warning message");
            } else {
                utilities::printf("Pass one of the following commands as input: warn, wspec, err, espec\n");
            }
        } else {
            utilities::printf("Pass one of the following commands as input: warn, wspec, err, espec\n");
        }
    }
};