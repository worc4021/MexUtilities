#include "mex.hpp"
#include "mexAdapter.hpp"
#include "sparse.hpp"


enum class commands {
    set,
    update,
    find,
    values,
    unknown
};

commands getcommand(const std::string& command) {
    std::string cmd(command);
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), [](unsigned char c){ return std::tolower(c); });
    if (0 == cmd.compare("set"))
        return commands::set;
    else if (0 == cmd.compare("update"))
        return commands::update;
    else if (0 == cmd.compare("find"))
        return commands::find;
    else if (0 == cmd.compare("values"))
        return commands::values;
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
        if (inputs.size() < 1)
            utilities::error("Command must be passed");
        matlab::data::Array mxcmd = std::move(inputs[0]);
        std::string command = utilities::getstringvalue(mxcmd);
        commands cmd = getcommand(command);
        matlab::data::ArrayFactory factory;

        switch (cmd) {
            case commands::set: {
                if (inputs.size() < 2)
                    utilities::error("A sparse matrix must be passed to set.");
                if (utilities::issparse(inputs[1])){
                    matlab::data::SparseArray<double> Amex = std::move(inputs[1]);
                    A.set(Amex);
                } else {
                    matlab::data::TypedArray<double> Amex = std::move(inputs[1]);
                    A.set(Amex);
                }
                if (outputs.size())
                    outputs[0] = A.get();
                break;
            }
            case commands::update: {
                if (inputs.size() < 2)
                    utilities::error("A sparse matrix must be passed to update.");
                matlab::data::SparseArray<double> Amex = std::move(inputs[1]);
                A.updateValues(Amex);
                if (outputs.size())
                    outputs[0] = A.get();
                break;
            }
            case commands::find: {
                matlab::data::buffer_ptr_t<int> iRow_p = factory.createBuffer<int>(A.getNumberOfNonZeroElements());
                matlab::data::buffer_ptr_t<int> jCol_p = factory.createBuffer<int>(A.getNumberOfNonZeroElements());
                A.iRow<int>(iRow_p.get());
                A.jCol<int>(jCol_p.get());
                if (outputs.size()) {
                    outputs[0] = factory.createArrayFromBuffer<int>({ A.getNumberOfNonZeroElements(),1 }, std::move(iRow_p));
                    if (outputs.size() > 1) 
                        outputs[1] = factory.createArrayFromBuffer<int>({ A.getNumberOfNonZeroElements(), 1 }, std::move(jCol_p));
                }
                break;
            }
            case commands::values: {
                matlab::data::buffer_ptr_t<double> val_p = factory.createBuffer<double>(A.getNumberOfNonZeroElements());
                A.val<double>(val_p.get());
                if (outputs.size())
                    outputs[0] = factory.createArrayFromBuffer<double>({ A.getNumberOfNonZeroElements(),1 }, std::move(val_p));
                break;
            }
            case commands::unknown: {
                utilities::error("unknown command passed.");
                break;
            }
        }
        
    }
};