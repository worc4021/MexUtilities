#include "mex.hpp"
#include "mexAdapter.hpp"
#include "utilities.hpp"

// getnested(s, 'a.b[1,2].c'[, fortranIndex]) -> value of the nested field
class MexFunction
    : public matlab::mex::Function
{

public:
    MexFunction()
    {
        matlabPtr = getEngine();
    }
    ~MexFunction() = default;
    void operator()(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        if (inputs.size() < 2)
            utilities::error("Call with getnested(struct, fieldpath[, fortranIndex])");

        if (!utilities::isstruct(inputs[0]))
            utilities::error("First input must be a struct");

        matlab::data::StructArray str = std::move(inputs[0]);
        std::string path = utilities::getstringvalue(inputs[1]);

        bool fortranIndex = false;
        if (inputs.size() > 2)
            fortranIndex = static_cast<bool>(matlab::data::TypedArray<bool>(inputs[2])[0]);

        auto field = utilities::get_nested_field(str, path, fortranIndex);
        outputs[0] = matlab::data::Array(field);
    }
};
