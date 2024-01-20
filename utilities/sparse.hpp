#pragma once
#include "utilities.hpp"
#include <type_traits>
#include <concepts>

namespace utilities {

template<std::floating_point T>
class Sparse {
private:
    std::size_t m{}, n{};
    std::vector<std::size_t> iOffset;
    std::vector<T> values;

    inline std::size_t linearIndex(std::size_t i, std::size_t j) const {
        return i + j * m;
    }

public:
    Sparse() = default;
    ~Sparse() = default;
    Sparse(std::size_t m, std::size_t n) : m(m), n(n) {}
    Sparse(const Sparse<T>& A) = default;
    Sparse(Sparse<T>&& A) = default;


    std::size_t getNumberOfRows() const { return m; }
    std::size_t getNumberOfColumns() const { return n; }
    std::size_t getNumberOfNonZeroElements() const { return values.size(); }

    void set(const matlab::data::SparseArray<T>& A) {
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

    void updateValues(const matlab::data::SparseArray<T>& B) {
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

    template<std::integral Index>
    void iRow(Index* rowPtr) const {
        std::transform(iOffset.cbegin(), iOffset.cend(), rowPtr, [&](std::size_t elem) { return elem % m; });
    }

    template<std::integral Index>
    void jCol(Index* colPtr) const {
        std::transform(iOffset.cbegin(), iOffset.cend(), colPtr, [&](std::size_t elem) { return elem / m; });
    }

    template<std::floating_point Number>
    void val(Number* valPtr) const {
        std::transform(values.cbegin(), values.cend(), valPtr, [](T elem) { return elem; });
    }

    matlab::data::SparseArray<T> get() const
    {
        matlab::data::ArrayFactory factory;

        std::size_t nnz = getNumberOfNonZeroElements();

        auto data_p = factory.createBuffer<T>(nnz);
        auto rows_p = factory.createBuffer<size_t>(nnz);
        auto cols_p = factory.createBuffer<size_t>(nnz);

        T *dataPtr = data_p.get();
        iRow(rows_p.get());
        jCol(cols_p.get());
        std::for_each(values.cbegin(), values.cend(), [&](const T &e)
                      { *(dataPtr++) = e; });
        
        matlab::data::SparseArray<T> A = factory.createSparseArray<T>({m, n}, nnz, std::move(data_p),
                                                                      std::move(rows_p), std::move(cols_p));
        return A;
    }
};

} // namespace utilities