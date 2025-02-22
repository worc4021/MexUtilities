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
        
        std::filesystem::path retval = utilities::getMexPath();

        matlab::data::ArrayFactory factory;
        
        outputs[0] = factory.createScalar(retval.string());
        
    }
};