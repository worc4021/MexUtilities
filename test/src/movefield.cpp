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
            matlab::data::ArrayFactory f;
            matlab::data::StructArray retval = f.createStructArray({1,1},{"hello"});
            matlab::data::StructArray field = f.createStructArray({1,1},{"world"});
            matlab::data::StructArray hello = utilities::movefield((matlab::data::StructArray &)inputs[0],"hello");
            field[0]["world"] = utilities::movefield(hello,"world");
            retval[0]["hello"] = std::move(field);
            outputs[0] = std::move(retval);
        } else {
            utilities::error("Call with struct('hello',struct('world',<value>))\n");
        }
    }
};