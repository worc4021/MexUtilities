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
        if (inputs.size() < 1)
            utilities::error("Not enough input arguments");
        
        if (!utilities::isstruct(inputs[0]))
            utilities::error("First input must be a struct");

        auto mxtoplevel = utilities::getfield(inputs[0], "toplevel");

        auto mxnestedlevels = utilities::getfield(inputs[0], "toplevel.nested");
        
        matlab::data::ArrayFactory factory;
        bool retval = true;
        outputs[0] = factory.createScalar(retval);
    }
};