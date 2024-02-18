#pragma once
#if defined(MATLAB_MEX_FILE)
#include "utilities.hpp"
#endif // defined(MATLAB_MEX_FILE)
#include <algorithm>
#include <vector>
#include <type_traits>
#include <concepts>
#include <span>

namespace utilities {

template<std::floating_point Number>
class Sparse {
private:
    std::size_t m{}, n{};
    std::vector<std::size_t> iOffset;
    std::vector<Number> values;

    inline std::size_t linearIndex(std::size_t i, std::size_t j) const {
        return i + j * m;
    }

public:
    Sparse() = default;
    ~Sparse() = default;
    Sparse(std::size_t m, std::size_t n) : m(m), n(n) {}
    Sparse(const Sparse<Number>& A) = default;
    Sparse(Sparse<Number>&& A) = default;
    Sparse operator=(const Sparse&) = delete;


    std::size_t getNumberOfRows() const { return m; }
    std::size_t getNumberOfColumns() const { return n; }
    std::size_t getNumberOfNonZeroElements() const { return values.size(); }


    template<std::integral Index>
    void iRow(Index* rowPtr) const {
        std::transform(iOffset.cbegin(), iOffset.cend(), rowPtr, [&](std::size_t elem) { return elem % m; });
    }

    template<std::integral Index>
    void iRow(std::span<Index> rowSpan) const {
        std::transform(iOffset.cbegin(), iOffset.cend(), rowSpan.begin(), [&](std::size_t elem) { return elem % m; });
    }

    template<std::integral Index>
    void jCol(Index* colPtr) const {
        std::transform(iOffset.cbegin(), iOffset.cend(), colPtr, [&](std::size_t elem) { return elem / m; });
    }

    template<std::integral Index>
    void jCol(std::span<Index> colSpan) const {
        std::transform(iOffset.cbegin(), iOffset.cend(), colSpan.begin(), [&](std::size_t elem) { return elem / m; });
    }

    void val(Number* valPtr) const {
        std::transform(values.cbegin(), values.cend(), valPtr, [](Number elem) { return elem; });
    }

    void val(std::span<Number> valSpan) const {
        std::transform(values.cbegin(), values.cend(), valSpan.begin(), [](Number elem) { return elem; });
    }

    template<std::integral Index>
    void getCsc(std::span<Index> columnBounds, std::span<Index> iRow, std::span<Number> val) const {
        std::vector<Index> columnProxy(n, 0);
        
        for (std::size_t i = 0; i < iOffset.size(); i++) {
            Index idx = iOffset.at(i);
            columnProxy.at(idx/m) += 1;
            iRow[i] = idx % m;
            val[i] = values.at(i);
        }
        std::fill(columnBounds.begin(), columnBounds.end(), 0);

        for (Index i = 0; i < n; i++) {
            columnBounds[i+1] = columnBounds[i] + columnProxy.at(i);
        }
    }

    template<std::integral Index>
    void set(std::span<Index> const iRow, std::span<Index> const jCol, std::span<Number> const val) {
        if (!iOffset.empty())
            iOffset.clear();
        if (!values.empty())
            values.clear();
        
        iOffset.resize(val.size());
        values.resize(val.size());

        for (std::size_t k = 0; k < val.size(); k++) {
            iOffset.at(k) = linearIndex(iRow[k], jCol[k]);
            values.at(k) = val[k];
        }
    }

#if defined(MATLAB_MEX_FILE)
    void set(const matlab::data::SparseArray<Number>& A) {
        if (!iOffset.empty())
            iOffset.clear();
        if (!values.empty())
            values.clear();
        m = A.getDimensions()[0];
        n = A.getDimensions()[1];
        iOffset.resize(A.getNumberOfNonZeroElements());
        values.resize(A.getNumberOfNonZeroElements());

        std::size_t k = 0;
        matlab::data::SparseIndex idx;
        for (auto it = A.cbegin(); it != A.cend(); it++) {
            idx = A.getIndex(it);
            iOffset.at(k) = linearIndex(idx.first, idx.second);
            values.at(k) = *it;
            k++;
        }
    }

    void set(const matlab::data::TypedArray<Number>& A) {
        if (!iOffset.empty())
            iOffset.clear();
        if (!values.empty())
            values.clear();
        m = A.getDimensions()[0];
        n = A.getDimensions()[1];
        
        for (std::size_t jCol = 0; jCol < n; jCol++) {
            for (std::size_t iRow = 0; iRow < m; iRow++) {
                if (std::abs(A[iRow][jCol]) > std::numeric_limits<Number>::epsilon()) {
                    iOffset.push_back(linearIndex(iRow,jCol));
                    values.push_back(A[iRow][jCol]);
                }
            }
        }
    }

    void set(const matlab::data::Array& A) {
        if (utilities::issparse(A)) {
            matlab::data::SparseArray<Number> B(A);
            set(B);
        } else {
            matlab::data::TypedArray<Number> B(A);
            set(B);
        }
    }

    void updateValues(const matlab::data::SparseArray<Number>& B) {
        std::size_t kA = 0;
        matlab::data::SparseIndex idx;
        std::size_t lidxB;
        for (auto it = B.cbegin(); it != B.cend(); it++) {
            idx = B.getIndex(it);
            lidxB = linearIndex(idx.first, idx.second);
            while (kA < iOffset.size() && lidxB > iOffset.at(kA)) {
                values.at(kA++) = 0.;
            }
            if (kA < iOffset.size() && lidxB == iOffset.at(kA)) {
                values.at(kA++) = *it;
            }
        }
        while (kA < iOffset.size()) {
            values.at(kA++) = 0.;
        }
    }

    matlab::data::SparseArray<Number> get() const
    {
        matlab::data::ArrayFactory factory;

        std::size_t nnz = getNumberOfNonZeroElements();

        auto data_p = factory.createBuffer<Number>(nnz);
        auto rows_p = factory.createBuffer<size_t>(nnz);
        auto cols_p = factory.createBuffer<size_t>(nnz);

        Number *dataPtr = data_p.get();
        iRow(rows_p.get());
        jCol(cols_p.get());
        std::for_each(values.cbegin(), values.cend(), [&](const Number &e)
                      { *(dataPtr++) = e; });
        
        matlab::data::SparseArray<Number> A = factory.createSparseArray<Number>({m, n}, nnz, std::move(data_p),
                                                                      std::move(rows_p), std::move(cols_p));
        return A;
    }
#endif // defined(MATLAB_MEX_FILE)    
};

} // namespace utilities