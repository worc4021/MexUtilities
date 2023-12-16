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
    void operator()(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs) {
        if (inputs.size() < 2)
            utilities::error("Not enough input arguments");
        
        if (!utilities::isstruct(inputs[0]))
            utilities::error("First input must be a struct");

        if (!utilities::isstring(inputs[1]))
            utilities::error("Second input must be a {}.","string");

        auto field = utilities::getstringvalue(inputs[1]);

        matlab::data::ArrayFactory factory;
        auto retval = utilities::isfield(inputs[0], field);
        outputs[0] = factory.createScalar(retval);
        
    }
};