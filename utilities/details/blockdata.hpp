#ifndef BLOCKDATA_HPP
#define BLOCKDATA_HPP
#if defined(MATLAB_MEX_FILE)
#include "utilities.hpp"
#endif // defined(MATLAB_MEX_FILE)
#include <vector>
#include <array>
#include <algorithm>
#include <numeric>
#include <exception>

namespace utilities::details {

template <std::size_t N, typename T>
class BlockData {
    static_assert(N > 0 && N < 4, "Invalid number of dimensions.");
    std::vector<T> _data;
    std::array<std::size_t, N> _dims;
public:
    BlockData() = default;
    BlockData(std::size_t nElements) : _data(nElements), _dims{nElements}, column(_dims, _data.data()), row(_dims, _data.data()), page(_dims, _data.data()), tensorial(_dims, _data.data()) {
        static_assert(N == 1, "Invalid number of dimensions");
    }

    BlockData(std::size_t nRows, std::size_t nCols) : _data(nRows * nCols), _dims{nRows, nCols}, column(_dims, _data.data()), row(_dims, _data.data()), page(_dims, _data.data()), tensorial(_dims, _data.data()) {
        static_assert(N == 2, "Invalid number of dimensions");
    }

    BlockData(std::size_t nRows, std::size_t nCols, std::size_t nPages) : _data(nRows * nCols * nPages), _dims{nRows, nCols, nPages}, column(_dims, _data.data()), row(_dims, _data.data()), page(_dims, _data.data()), tensorial(_dims, _data.data()) {
        static_assert(N == 3, "Invalid number of dimensions");
    }

    BlockData(std::array<std::size_t, N> dims) : _data(std::accumulate(dims.begin(), dims.end(), 1, std::multiplies<std::size_t>())), _dims(dims), column(_dims, _data.data()), row(_dims, _data.data()), page(_dims, _data.data()), tensorial(_dims, _data.data()) {
        static_assert(N > 0 && N < 4, "Invalid number of dimensions");
    }

    BlockData(const BlockData&) = default;
    BlockData(BlockData&&) = default;
    ~BlockData() = default;
#if defined(MATLAB_MEX_FILE)
    BlockData(matlab::data::Array&& A) 
        : _data(A.getNumberOfElements())
        , _dims()
        , column(_dims, _data.data())
        , row(_dims, _data.data())
        , page(_dims, _data.data())
        , tensorial(_dims, _data.data()) {
        matlab::data::TypedArray<T> A_typed(std::move(A));
        std::copy_n(A_typed.getDimensions().begin(), N, _dims.begin());
        column.resize(_dims, _data.data());
        row.resize(_dims, _data.data());
        page.resize(_dims, _data.data());
        tensorial.resize(_dims, _data.data());
        std::copy(A_typed.begin(), A_typed.end(), _data.begin());
    }
#endif // defined(MATLAB_MEX_FILE)
    
    BlockData& operator=(const BlockData&) = default;
    BlockData& operator=(BlockData&&) = default;

    BlockData &resize(std::size_t nElements) {
        static_assert(N == 1, "Invalid number of dimensions");
        _data.resize(nElements);
        _dims.at(0) = nElements;
        column.resize(_dims, _data.data());
        row.resize(_dims, _data.data());
        page.resize(_dims, _data.data());
        tensorial.resize(_dims, _data.data());
        return *this;
    }

    BlockData& resize(std::size_t nRows, std::size_t nCols) {
        static_assert(N == 2, "Invalid number of dimensions");
        _data.resize(nRows * nCols);
        _dims.at(0) = nRows;
        _dims.at(1) = nCols;
        column.resize(_dims, _data.data());
        row.resize(_dims, _data.data());
        page.resize(_dims, _data.data());
        tensorial.resize(_dims, _data.data());
        return *this;
    }

    BlockData& resize(std::size_t nRows, std::size_t nCols, std::size_t nPages) {
        static_assert(N == 3, "Invalid number of dimensions");
        _data.resize(nRows * nCols * nPages);
        _dims.at(0) = nRows;
        _dims.at(1) = nCols;
        _dims.at(2) = nPages;
        column.resize(_dims, _data.data());
        row.resize(_dims, _data.data());
        page.resize(_dims, _data.data());
        tensorial.resize(_dims, _data.data());
        return *this;
    }

    std::size_t size() const {
        return _data.size();
    }

    std::size_t nRows() const {
        static_assert(N >= 1, "Invalid number of dimensions");
        return _dims.at(0);
    }

    std::size_t nCols() const {
        static_assert(N >= 2, "Invalid number of dimensions");
        return _dims.at(1);
    }

    std::size_t nPages() const {
        static_assert(N > 2, "Invalid number of dimensions");
        return _dims.at(2);
    }

    T &operator[](std::size_t iElement) {
        static_assert(N == 1, "Invalid number of dimensions");
        return _data.at(iElement);
    }

    const T &operator[](std::size_t iElement) const {
        return static_cast<const T&>(const_cast<BlockData<N,T>*>(*this)->operator[](iElement));
    }

    T &operator()(std::size_t iRow, std::size_t jCol) {
        static_assert(N == 2, "Invalid number of dimensions");
        return _data.at(iRow + jCol * _dims[0]);
    }

    const T &operator()(std::size_t iRow, std::size_t jCol) const {
        return static_cast<const T&>(const_cast<BlockData<N,T>*>(*this)->operator()(iRow, jCol));
    }

    T &operator()(std::size_t iRow, std::size_t jCol, std::size_t kPage) {
        static_assert(N == 3, "Invalid number of dimensions");
        return _data.at(iRow + jCol * _dims[0] + kPage * _dims[0] * _dims[1]);
    }

    const T &operator()(std::size_t iRow, std::size_t jCol, std::size_t kPage) const {
        return static_cast<const T&>(const_cast<BlockData<N,T>*>(*this)->operator()(iRow, jCol, kPage));
    }

    struct Iterator {
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = T*;
        using reference = T&;

        Iterator(pointer ptr, std::size_t stride = 1ULL) : _ptr(ptr), _stride(stride) {}
        reference operator*() const { return *_ptr; }
        pointer operator->() const { return _ptr; }
        Iterator& operator++() {
            _ptr += _stride;
            return *this;
        }
        Iterator operator++(int) {
            Iterator tmp = *this;
            for(std::size_t i = 0; i < _stride; ++i) {
                ++(*this);
            }
            return tmp;
        }
        Iterator& operator--() {
            _ptr -= _stride;
            return *this;
        }
        Iterator operator--(int) {
            Iterator tmp = *this;
            for(std::size_t i = 0; i < _stride; ++i) {
                --(*this);
            }
            return tmp;
        }
        bool operator==(const Iterator& other) const { return _ptr == other._ptr; }
        bool operator!=(const Iterator& other) const { return _ptr != other._ptr; }
        private:
        pointer _ptr;
        std::size_t _stride;
    };

    using iterator = Iterator;
    using const_iterator = Iterator;

    iterator begin() { return Iterator(_data.data()); }
    iterator end() { return Iterator(_data.data() + _data.size()); }

    const_iterator cbegin() const { return const_cast<BlockData<N,T>*>(this)->begin(); }
    const_iterator cend() const { return const_cast<BlockData<N,T>*>(this)->end(); }

    class Column {
        std::size_t currentColumn{};
        std::size_t offset{0};
        std::array<std::size_t, N> _dims{{}};
        T* _data{nullptr};
        friend class BlockData;
        friend class Page;
        Column& resize(std::array<std::size_t, N> newDims, T* newData) {
            _dims = newDims;
            _data = newData;
            return *this;
        }
    public:
        Column() = default;
        Column(std::array<std::size_t, N> dims, T* data) : currentColumn{0ULL}, offset{0ULL}, _dims(dims), _data(data) {}
        Column &operator()(std::size_t jCol) {
            static_assert(N >= 2, "Invalid number of dimensions");
            if (jCol >= _dims.at(1)) {
                throw std::out_of_range("Column index out of range");
            }
            currentColumn = jCol;
            return *this;
        }
        const Column &operator()(std::size_t jCol) const {
            return const_cast<Column*>(this)->operator()(jCol);
        }
        Iterator begin() { return Iterator(_data + offset + currentColumn*_dims.at(0)); }
        Iterator end() { return Iterator(_data + offset + (currentColumn + 1) * _dims.at(0)); }

        const Iterator begin() const { return const_cast<Column*>(this)->begin(); }
        const Iterator end() const { return const_cast<Column*>(this)->end(); }

        T* data() {
            return _data + offset + currentColumn * _dims.at(0);
        }
        const T* data() const {
            return const_cast<Column*>(this)->data();
        }

        std::size_t size() const { return _dims.at(0); }
    } column;

    class Row {
        std::size_t currentRow {};
        std::size_t offset{0}; // Offset for multi-dimensional data
        std::array<std::size_t, N> _dims{{}};
        T* _data{nullptr};
        friend class BlockData;
        friend class Page;
        Row& resize(std::array<std::size_t, N> newDims, T* newData) {
            _dims = newDims;
            _data = newData;
            return *this;
        }
        public:
        Row() = default;
        Row(std::array<std::size_t, N> dims, T* data) : currentRow{0ULL}, offset{0ULL}, _dims(dims), _data(data) {}
        Row &operator()(std::size_t iRow) {
            if (iRow >= _dims.at(0)) {
                throw std::out_of_range("Row index out of range");
            }
            currentRow = iRow;
            return *this;
        }
        const Row &operator()(std::size_t iRow) const {
            return const_cast<Row*>(this)->operator()(iRow);
        }

        iterator begin() {
            if constexpr (N == 1) {
                return iterator(_data + currentRow);
            } else if constexpr (N == 2) {
                return iterator(_data + currentRow, _dims.at(0));
            } else {
                // Row on page offset
                return iterator(_data + currentRow + offset , _dims.at(0));
            }
        }

        iterator end() {
            if constexpr (N == 1) {
                return iterator(_data + _dims.at(0));
            } else if constexpr (N == 2) {
                return iterator(_data + currentRow + _dims.at(0) * _dims.at(1), _dims.at(0));
            } else {
                return iterator(_data + currentRow + offset + _dims.at(0) * _dims.at(1), _dims.at(0));
            }
        }

        const_iterator cbegin() const {
            return const_cast<Row*>(this)->begin();
        }
        const_iterator cend() const {
            return const_cast<Row*>(this)->end();
        }

        std::size_t size() const {
            static_assert(N >= 1, "Invalid number of dimensions");
            return _dims[1];
        }
    } row;

    class Page {
        std::size_t currentPage {};
        std::array<std::size_t, N> _dims{{}};
        T* _data{nullptr};
        friend class BlockData;
        Page& resize(std::array<std::size_t, N> newDims, T* newData) {
            _dims = newDims;
            _data = newData;
            row.resize(newDims, newData);
            column.resize(newDims, newData);
            return *this;
        }

        public:
        Page() = default;
        Page(std::array<std::size_t, N> dims, T* data) : currentPage{0ULL}, _dims(dims), _data(data), row(_dims, _data), column(_dims, _data) {}
        Page &operator()(std::size_t kPage) {
            static_assert(N > 2, "Invalid number of dimensions");
            if (kPage >= _dims.at(2)) {
                throw std::out_of_range("Page index out of range");
            }
            currentPage = kPage;
            row.offset = currentPage * _dims.at(0) * _dims.at(1);
            column.offset = currentPage * _dims.at(0) * _dims.at(1);
            return *this;
        }
        const Page &operator()(std::size_t kPage) const {
            return const_cast<Page*>(this)->operator()(kPage);
        }

        iterator begin() {
            return iterator(_data + currentPage * _dims.at(0) * _dims.at(1));
        }
        iterator end() {
            return iterator(_data + (currentPage + 1) * _dims.at(0) * _dims.at(1));
        }

        const_iterator cbegin() const {
            return const_cast<Page*>(this)->begin();
        }
        const_iterator cend() const {
            return const_cast<Page*>(this)->end();
        }

        T* data() {
            return _data + currentPage * _dims.at(0) * _dims.at(1);
        }
        const T* data() const {
            return const_cast<Page*>(this)->data();
        }
        Row row;
        Column column;
        std::size_t size() const {
            if constexpr (N < 3) {
                return 1ULL;
            } else {
                return _dims.at(0) * _dims.at(1);
            }
        }
    } page;

    class TensorDirection {
        std::size_t iRow{}, jCol{};
        std::array<std::size_t, N> _dims{{}};
        T* _data{nullptr};
        friend class BlockData;
        TensorDirection& resize(std::array<std::size_t, N> newDims, T* newData) {
            _dims = newDims;
            _data = newData;
            return *this;
        }
        public:
        TensorDirection() = default;
        TensorDirection(std::array<std::size_t, N> dims, T* data) : iRow{0ULL}, jCol{0ULL}, _dims(dims), _data(data) {}
        TensorDirection &operator()(std::size_t iRow, std::size_t jCol) {
            static_assert(N >= 2, "Invalid number of dimensions");
            if (iRow >= _dims.at(0) || jCol >= _dims.at(1)) {
                throw std::out_of_range("Tensor direction indices out of range");
            }
            this->iRow = iRow;
            this->jCol = jCol;
            return *this;
        }
        const TensorDirection &operator()(std::size_t iRow, std::size_t jCol) const {
            return const_cast<TensorDirection*>(this)->operator()(iRow, jCol);
        }
        iterator begin() {
            static_assert(N >= 2, "Invalid number of dimensions");
            return iterator(_data + iRow + jCol * _dims.at(0), _dims.at(0) * _dims.at(1));
        }
        iterator end() {
            static_assert(N >= 2, "Invalid number of dimensions");
            return iterator(_data + iRow + jCol * _dims.at(0) + _dims.at(0) * _dims.at(1) * _dims.at(2), _dims.at(0) * _dims.at(1));
        }
        const_iterator cbegin() const {
            return const_cast<TensorDirection*>(this)->begin();
        }
        const_iterator cend() const {
            return const_cast<TensorDirection*>(this)->end();
        }
    } tensorial;

};


} // namespace utilities::details
#endif // BLOCKDATA_HPP