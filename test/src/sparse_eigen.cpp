#include "mex.hpp"
#include "mexAdapter.hpp"
#include "eigen/sparse.hpp"



enum class commands {
    toeigen,
    fromeigen,
    unknown
};

commands getcommand(const std::string& command) {
    std::string cmd(command);
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), [](unsigned char c){ return std::tolower(c); });
    if (0 == cmd.compare("toeigen"))
        return commands::toeigen;
    else if (0 == cmd.compare("fromeigen"))
        return commands::fromeigen;
    else
        utilities::error("Unknown command");
    return commands::unknown;
}

class MexFunction 
    : public matlab::mex::Function 
{
public:
    Eigen::SparseMatrix<double> A;
    MexFunction()  {
        matlabPtr = getEngine();
    }
    ~MexFunction() = default;
    void operator()(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs) {
        if (inputs.size() < 1)
            utilities::error("Command must be passed");
        matlab::data::Array mxcmd = std::move(inputs[0]);
        std::string command = utilities::getstringvalue(mxcmd);
        commands cmd = getcommand(command);
        matlab::data::ArrayFactory factory;

        switch (cmd) {
            case commands::toeigen: {
                if (inputs.size() < 2)
                    utilities::error("A sparse matrix must be passed to set.");
                if (utilities::issparse(inputs[1])){
                    matlab::data::SparseArray<double> Amex = std::move(inputs[1]);
                    A = utilities::eigen::toEigen(Amex);
                }
                break;
            }
            case commands::fromeigen: {
                matlab::data::SparseArray<double> Amex = utilities::eigen::toMatlab<double>(A);
                if (outputs.size())
                    outputs[0] = std::move(Amex);
                break;
            }
            case commands::unknown: {
                utilities::error("unknown command passed.");
                break;
            }
        }
        
    }
};