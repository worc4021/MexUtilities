#pragma once
#include "MatlabDataArray.hpp"
#include "utilities.hpp"
#if defined(USE_EIGEN)
#include <Eigen/Dense>

namespace utilities::eigen {

inline Eigen::MatrixXd convert(const matlab::data::Array& x) {
    matlab::data::TypedArray<double> mat(std::move(x));
    matlab::data::ArrayDimensions dims(mat.getDimensions());
    Eigen::MatrixXd retVal(dims[0], dims[1]);

    for (auto i = 0; i < retVal.rows(); i++) {
        for (auto j = 0; j < retVal.cols(); j++){
            retVal(i,j) = mat[i][j];
        }
    }
    
    return retVal;
}

inline matlab::data::TypedArray<double> convert(const Eigen::MatrixXd& x) {
    matlab::data::ArrayFactory factory;
    matlab::data::ArrayDimensions dims({static_cast<size_t>(x.rows()),static_cast<size_t>(x.cols())});
    matlab::data::buffer_ptr_t<double> buffer =  factory.createBuffer<double>(static_cast<size_t>(x.rows()*x.cols()));
    
    for (auto j = 0; j<x.cols(); j++)
        for (auto i = 0; i<x.rows(); i++)
            buffer[i+j*x.rows()] = x(i,j);

    matlab::data::TypedArray<double> retVal = factory.createArrayFromBuffer<double>(dims, std::move(buffer));
    return retVal;
}

} // namespace utilities::eigen

#endif // defined(USE_EIGEN)