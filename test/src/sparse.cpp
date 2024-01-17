#include "mex.hpp"
#include "mexAdapter.hpp"
#include "sparse.hpp"


enum class commands {
    set,
    update,
    unknown
};

commands getcommand(const std::string& command) {
    std::string cmd(command);
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), [](unsigned char c){ return std::tolower(c); });
    if (0 == cmd.compare("set"))
        return commands::set;
    else if (0 == cmd.compare("update"))
        return commands::update;
    else
        utilities::error("Unknown command");
        return commands::unknown;
}

class MexFunction 
    : public matlab::mex::Function 
{
    utilities::Sparse<double> A;
public:
    MexFunction()  {
        matlabPtr = getEngine();
    }
    ~MexFunction() = default;
    void operator()(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs) {
        if (inputs.size() < 2)
            utilities::error("Not enough input arguments");
        matlab::data::Array mxcmd = std::move(inputs[0]);
        std::string command = utilities::getstringvalue(mxcmd);
        commands cmd = getcommand(command);
        
        matlab::data::SparseArray<double> Amex = std::move(inputs[1]);

        switch (cmd) {
            case commands::set:
                A.set(Amex);
                break;
            case commands::update:
                A.updateValues(Amex);
                break;
        }
        
        outputs[0] = A.get();
        
    }
};