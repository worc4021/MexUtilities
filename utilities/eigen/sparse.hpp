#pragma once
#include "MatlabDataArray.hpp"
#include "utilities.hpp"

#include <Eigen/SparseCore>

namespace utilities::eigen {

template<std::floating_point Number>
Eigen::SparseMatrix<Number> toEigen(const matlab::data::SparseArray<Number>& A) {
    Eigen::SparseMatrix<Number> retVal(A.getDimensions()[0], A.getDimensions()[1]);
    matlab::data::SparseIndex idx;
    for (auto it = A.cbegin(); it != A.cend(); ++it) {
        idx = A.getIndex(it);
        retVal.insert(idx.first, idx.second) = *it;
    }
    retVal.makeCompressed();
    return retVal;
}

template<std::floating_point Number>
matlab::data::SparseArray<Number> toMatlab(const Eigen::SparseMatrix<Number>& A) {
    matlab::data::ArrayFactory factory;
    matlab::data::ArrayDimensions dims({static_cast<size_t>(A.rows()), static_cast<size_t>(A.cols())});
    matlab::data::buffer_ptr_t<std::size_t> buffer_iRow = factory.createBuffer<std::size_t>(A.nonZeros());
    matlab::data::buffer_ptr_t<std::size_t> buffer_jCol = factory.createBuffer<std::size_t>(A.nonZeros());
    matlab::data::buffer_ptr_t<Number> buffer_val = factory.createBuffer<Number>(A.nonZeros());
    std::size_t nnz = 0;
    for (int k = 0; k < A.outerSize(); ++k) {
        for (typename Eigen::SparseMatrix<Number>::InnerIterator it(A, k); it; ++it) {
            buffer_iRow[nnz] = it.row();
            buffer_jCol[nnz] = it.col();
            buffer_val[nnz] = it.value();
            ++nnz;
        }
    }
    return factory.createSparseArray<Number>(dims,nnz,std::move(buffer_val),std::move(buffer_iRow),std::move(buffer_jCol));
}

} // namespace utilities::eigen