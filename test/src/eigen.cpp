#include "mex.hpp"
#include "mexAdapter.hpp"
#include "eigen/conversions.hpp"

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
        
        if (!utilities::ismatrix(inputs[0]))
            utilities::error("First input a matrix");

        Eigen::MatrixXd x = utilities::eigen::convert(inputs[0]);
        
        Eigen::MatrixXd eye = Eigen::MatrixXd::Identity(x.rows(), x.cols());

        Eigen::MatrixXd y = x + eye;

        outputs[0] = utilities::eigen::convert(y);
    }
};