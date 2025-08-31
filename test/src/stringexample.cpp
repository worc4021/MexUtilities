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
        matlab::data::ArrayFactory factory;

        matlab::data::TypedArray<matlab::data::MATLABString> retval = 
        factory.createArray<matlab::data::MATLABString>({2,1}, {matlab::data::MATLABString(u"Hello"),matlab::data::MATLABString(u"World")});
        outputs[0] = std::move(retval);

        std::vector<std::u16string> inputStrings{u"hello", u"world"};
        matlab::data::TypedArray<matlab::data::MATLABString> inputArray
        = factory.createArray<matlab::data::MATLABString>({2,1});
        std::transform(inputStrings.begin(), inputStrings.end(), inputArray.begin(),
            [](const std::u16string& str) { return str; });
        inputs[0] = std::move(inputArray);
    }
};