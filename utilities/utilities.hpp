#ifndef MEX_UTILITIES_HPP
#define MEX_UTILITIES_HPP
#include <filesystem>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include "MatlabDataArray.hpp"
#include "cppmex/mexMatlabEngine.hpp"

#ifndef mex_hpp
extern std::shared_ptr<matlab::engine::MATLABEngine> matlabPtr;
#else
std::shared_ptr<matlab::engine::MATLABEngine> matlabPtr;
#endif

namespace utilities
{

    inline bool isstruct(const matlab::data::Array &x)
    {
        return (matlab::data::ArrayType::STRUCT == x.getType());
    }

    inline bool isfield(const matlab::data::StructArray &s, const std::string& fieldname)
    {
        matlab::data::StructArray str(s);
        auto fieldnames = str.getFieldNames();
        auto idx = fieldname.find_first_of('.');
        if (idx == std::string::npos)
        {
            return (std::find(fieldnames.begin(), fieldnames.end(), fieldname) != fieldnames.end());
        }
        else
        {
            std::string toplevel = fieldname.substr(0, idx);
            std::string nestedlevels = fieldname.substr(idx + 1);
            if (std::find(fieldnames.begin(), fieldnames.end(), toplevel) != fieldnames.end())
            {
                if (utilities::isstruct(str[0][toplevel]))
                    return utilities::isfield(str[0][toplevel], nestedlevels);
                else
                    return true;
            }
            else
            {
                return false;
            }
        }
    }

    inline bool isscalar(const matlab::data::Array &x)
    {
        return (1 == x.getNumberOfElements());
    }

    inline bool iswholenumber(double x)
    {
        return (0. == x - std::floor(x));
    }

    inline bool isinteger(const matlab::data::Array &x)
    {
        matlab::data::TypedArray<double> y(x);
        bool retVal = true;
        for (auto &elem : y)
            retVal &= utilities::iswholenumber(elem);
        return retVal;
    }

    inline bool isscalarinteger(const matlab::data::Array &x)
    {
        matlab::data::TypedArray<double> y(x);
        return (utilities::isinteger(y) && utilities::isscalar(y));
    }

    inline bool ispositive(const matlab::data::Array &x)
    {
        matlab::data::TypedArray<double> y(x);
        return 0 < y[0];
    }

    inline bool isstring(const matlab::data::Array &x)
    {
        return ((matlab::data::ArrayType::CHAR == x.getType()) || (matlab::data::ArrayType::MATLAB_STRING == x.getType()));
    }

    inline bool isnumeric(const matlab::data::Array &x)
    {
        bool retVal = false;
        for (int i = 3; i < 23; i++)
            retVal |= (matlab::data::ArrayType(i) == x.getType());
        return retVal;
    }

    inline bool ishandle(const matlab::data::Array &x)
    {
        return (matlab::data::ArrayType::HANDLE_OBJECT_REF == x.getType());
    }

    inline bool issparse(const matlab::data::Array &x)
    {
        return (matlab::data::ArrayType::SPARSE_DOUBLE == x.getType());
    }

    inline bool isvector(const matlab::data::Array &x)
    {
        auto dims = x.getDimensions();
        bool rowVector = (dims.size() == 2) && (dims[0] == 1);
        bool colVector = (dims.size() == 2) && (dims[1] == 1);
        return isnumeric(x) && (rowVector || colVector);
    }

    inline bool ismatrix(const matlab::data::Array &x)
    {
        auto dims = x.getDimensions();
        return isnumeric(x) && (dims.size() == 2);
    }

    inline std::string getstringvalue(const matlab::data::Array &x)
    {
        if (matlab::data::ArrayType::CHAR == x.getType())
        {
            matlab::data::CharArray retVal(x);
            return retVal.toAscii();
        }
        else if (matlab::data::ArrayType::MATLAB_STRING == x.getType())
        {
            matlab::data::StringArray retVal(x);
            return retVal[0];
        }
        return std::string("");
    }

    template <typename... _Args>
    void errWithId(const std::string& errorMnemonic, fmt::format_string<_Args...> __fmt, _Args &&...__args)
    {
        std::string errorId = fmt::format("{}:{}", TOOLNAME, errorMnemonic);
        std::string message = fmt::format(__fmt, std::forward<_Args>(__args)...);
        matlab::data::ArrayFactory factory;
        matlabPtr->feval(
            matlab::engine::convertUTF8StringToUTF16String("error"),
            0,
            std::vector<matlab::data::Array>({factory.createScalar(errorId), factory.createScalar(message)}));
    }

    template <typename... _Args>
    void error(fmt::format_string<_Args...> __fmt, _Args &&...__args)
    {
        errWithId("unspecific", __fmt, std::forward<_Args>(__args)...);
    }

    template <typename... _Args>
    void warnWithId(const std::string& warningMnemonic, fmt::format_string<_Args...> __fmt, _Args &&...__args)
    {
        std::string message = fmt::format(__fmt, std::forward<_Args>(__args)...);
        std::string warningId = fmt::format("{}:{}", TOOLNAME, warningMnemonic);
        matlab::data::ArrayFactory factory;
        matlabPtr->feval(
            matlab::engine::convertUTF8StringToUTF16String("warning"),
            0,
            std::vector<matlab::data::Array>({ factory.createScalar(warningId), factory.createScalar(message) }));
    }

    template <typename... _Args>
    void warning(fmt::format_string<_Args...> __fmt, _Args &&...__args) {
        warnWithId("unspecific", __fmt, std::forward<_Args>(__args)...);
    }

    template <typename... _Args>
    void printf(fmt::format_string<_Args...> __fmt, _Args &&...__args)
    {
        std::string message = fmt::format(__fmt, std::forward<_Args>(__args)...);
        matlab::data::ArrayFactory factory;
        matlabPtr->feval(
            matlab::engine::convertUTF8StringToUTF16String("fprintf"),
            0,
            std::vector<matlab::data::Array>({factory.createScalar(message)}));
    }

    inline matlab::data::Array getfield(const matlab::data::Array &s, std::string fieldname)
    {
        if (matlab::data::ArrayType::STRUCT != s.getType())
            utilities::error("getfield: input must be a struct");

        matlab::data::StructArray str(s);
        auto fieldnames = str.getFieldNames();
        auto idx = fieldname.find_first_of('.');
        if (idx == std::string::npos)
        {
            if (std::find(fieldnames.begin(), fieldnames.end(), fieldname) != fieldnames.end())
            {
                return str[0][fieldname];
            }
            else
            {
                utilities::error("getfield: {} not found.", fieldname);
            }
        }
        else
        {
            std::string toplevel = fieldname.substr(0, idx);
            std::string nestedlevels = fieldname.substr(idx + 1);
            if (std::find(fieldnames.begin(), fieldnames.end(), toplevel) != fieldnames.end())
            {
                return utilities::getfield(str[0][toplevel], nestedlevels);
            }
            else
            {
                utilities::error("getfield: {} not found.", fieldname);
            }
        }
        return str;
    }

    static matlab::data::ArrayRef get_nested_field(matlab::data::StructArray& str, const std::string_view field, bool fortranIndex = false) {
        auto count_occurances = [](const std::string_view str, char ch) {
            std::size_t count = 0;
            for (auto c : str) {
                if (c == ch) {
                    ++count;
                }
            }
            return count;
        };

        const auto nLevels = count_occurances(field, '.') + 1;
        
        std::size_t idx = 0;
        std::size_t offset{0};
        std::size_t parent_idx{0};
        if (1 == nLevels) {
            auto fieldname = field;
            if (std::string_view::npos != fieldname.find_first_of('[')) {
                idx = static_cast<std::size_t>(std::stoul(std::string(fieldname.substr(fieldname.find_first_of('[') + 1, fieldname.find_first_of(']') - fieldname.find_first_of('[') - 1))));
                fieldname = fieldname.substr(0, fieldname.find_first_of('['));
            }
            auto fieldnames = str.getFieldNames();
            if (std::find(fieldnames.begin(), fieldnames.end(), std::string(fieldname)) == fieldnames.end())
            {
                utilities::error("get_nested: invalid field name {}", fieldname);
            }
            return str[parent_idx][std::string(fieldname)];
        } else {
            auto fieldname = field.substr(0, field.find_first_of('.'));
            offset = fieldname.length() + 1;
            if (std::string_view::npos != fieldname.find_first_of('[')) {
                idx = static_cast<std::size_t>(std::stoul(std::string(fieldname.substr(fieldname.find_first_of('[') + 1, fieldname.find_first_of(']') - fieldname.find_first_of('[') - 1))));
                fieldname = fieldname.substr(0, fieldname.find_first_of('['));
            }
            auto fieldnames = str.getFieldNames();
            if (std::find(fieldnames.begin(), fieldnames.end(), std::string(fieldname)) == fieldnames.end())
            {
                utilities::error("get_nested: invalid field name {} on total field {}", fieldname, field);
            }
            if (parent_idx > str.getNumberOfElements())
            {
                utilities::error("get_nested: index {} out of bounds on field {} while processing {}", parent_idx, fieldname, field);
            }
            matlab::data::StructArrayRef recursiveField = str[parent_idx][std::string(fieldname)];
            parent_idx = idx;
            
            for (std::size_t i = 1; i < nLevels - 1; ++i) {
                fieldname = field.substr(offset, field.find_first_of('.', offset) - offset);
                offset += fieldname.length() + 1;
                idx = 0;
                if (std::string_view::npos != fieldname.find_first_of('[')) {
                    idx = static_cast<std::size_t>(std::stoul(std::string(fieldname.substr(fieldname.find_first_of('[') + 1, fieldname.find_first_of(']') - fieldname.find_first_of('[') - 1))));
                    if (fortranIndex) {
                        idx -= 1;
                    }
                    fieldname = fieldname.substr(0, fieldname.find_first_of('['));
                }
                auto fieldnames = recursiveField.getFieldNames();
                if (std::find(fieldnames.begin(), fieldnames.end(), std::string(fieldname)) == fieldnames.end())
                {
                    utilities::error("get_nested: invalid field name {} on total field {}", fieldname, field);
                }
                if (parent_idx >= recursiveField.getNumberOfElements())
                {
                    utilities::error("get_nested: index {} out of bounds on field {} while processing {}", parent_idx, fieldname, field);
                }
#if defined(__clang__) || defined(__GNUC__)
                recursiveField = static_cast<matlab::data::StructArrayRef>(recursiveField[parent_idx][std::string(fieldname)]);
#else
                recursiveField.operator=((matlab::data::StructArrayRef &)recursiveField[parent_idx][std::string(fieldname)]);
#endif
                parent_idx = idx;
            }
            
            fieldname = field.substr(offset, field.find_first_of('.', offset) - offset);
            idx = 0;
            if (std::string_view::npos != fieldname.find_first_of('[')) {
                idx = static_cast<std::size_t>(std::stoul(std::string(fieldname.substr(fieldname.find_first_of('[') + 1, fieldname.find_first_of(']') - fieldname.find_first_of('[') - 1))));
                if (fortranIndex) {
                    idx -= 1;
                }
                fieldname = fieldname.substr(0, fieldname.find_first_of('['));
            }
            fieldnames = recursiveField.getFieldNames();
            if (std::find(fieldnames.begin(), fieldnames.end(), std::string(fieldname)) == fieldnames.end())
            {
                utilities::error("get_nested: invalid field name {} on total field {}", fieldname, field);
            }
            if (parent_idx >= recursiveField.getNumberOfElements())
            {
                utilities::error("get_nested: index {} out of bounds on field {} while processing {}", parent_idx, fieldname, field);
            }
            return recursiveField[parent_idx][std::string(fieldname)];
        }
        
    }

// This function appears to have all sorts of issues but it functions correctly when tested, hence we mute the issues.
#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wreturn-type"
#   pragma clang diagnostic ignored "-Wreturn-stack-address"
#elif defined(_MSC_VER)
#   pragma warning (push)
#   pragma warning (disable: 4172)
#   pragma warning (disable: 4715)
#endif
    inline matlab::data::Array &&movefield(matlab::data::StructArray &str, const std::string &fieldname, std::size_t idx = 0) {
        auto fieldnames = str.getFieldNames();
        if (std::find(fieldnames.begin(), fieldnames.end(), fieldname) != fieldnames.end())
        {
            return std::move(str[idx][fieldname]);
        }
        else
        {
            utilities::error("movefield: {} not found.", fieldname);
        }
    }
#if defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(_MSC_VER)
#   pragma warning (pop)
#endif

    inline void addSingleField(matlab::data::Array &s, std::string fieldname, const matlab::data::Array value)
    {
        matlab::data::ArrayFactory factory;
        matlab::data::StructArray str(std::move(s));
        std::vector<std::string> fieldnames(str.getFieldNames().begin(), str.getFieldNames().end());
        fieldnames.push_back(fieldname);
        matlab::data::StructArray stmp = factory.createStructArray({1, 1}, fieldnames);
        for (const auto &field : str.getFieldNames())
        {
            // This beauty is needed for some reason on windows. On mac & linux
            // stmp[0][field] = std::move(str[0][field]);
            // works just fine. But on windows, it throws a compiler error.
            switch (str[0][field].getType())
            {
            case matlab::data::ArrayType::LOGICAL:
            {
                matlab::data::TypedArray<bool> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::CHAR:
            {
                matlab::data::CharArray tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::MATLAB_STRING:
            {
                matlab::data::StringArray tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::DOUBLE:
            {
                matlab::data::TypedArray<double> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::SINGLE:
            {
                matlab::data::TypedArray<float> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::INT8:
            {
                matlab::data::TypedArray<int8_t> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::UINT8:
            {
                matlab::data::TypedArray<uint8_t> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::INT16:
            {
                matlab::data::TypedArray<int16_t> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::UINT16:
            {
                matlab::data::TypedArray<uint16_t> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::INT32:
            {
                matlab::data::TypedArray<int32_t> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::UINT32:
            {
                matlab::data::TypedArray<uint32_t> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::INT64:
            {
                matlab::data::TypedArray<int64_t> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::UINT64:
            {
                matlab::data::TypedArray<uint64_t> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::COMPLEX_DOUBLE:
            {
                matlab::data::TypedArray<std::complex<double>> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::COMPLEX_SINGLE:
            {
                matlab::data::TypedArray<std::complex<float>> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::COMPLEX_INT8:
            {
                matlab::data::TypedArray<std::complex<int8_t>> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::COMPLEX_UINT8:
            {
                matlab::data::TypedArray<std::complex<uint8_t>> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::COMPLEX_INT16:
            {
                matlab::data::TypedArray<std::complex<int16_t>> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::COMPLEX_UINT16:
            {
                matlab::data::TypedArray<std::complex<uint16_t>> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::COMPLEX_INT32:
            {
                matlab::data::TypedArray<std::complex<int32_t>> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::COMPLEX_UINT32:
            {
                matlab::data::TypedArray<std::complex<uint32_t>> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::COMPLEX_INT64:
            {
                matlab::data::TypedArray<std::complex<int64_t>> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::COMPLEX_UINT64:
            {
                matlab::data::TypedArray<std::complex<uint64_t>> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::CELL:
            {
                matlab::data::CellArray tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::STRUCT:
            {
                matlab::data::StructArray tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::OBJECT:
            {
                matlab::data::ObjectArray tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::VALUE_OBJECT:
            {
                // not implemented because I'm not sure what this is.
                break;
            }
            case matlab::data::ArrayType::HANDLE_OBJECT_REF:
            {
                // Similarly, not sure what this is. Generally, we cannot "use"
                // matlab callbacks in a mex/cpp environment, instead we have to
                // call a string with feval and pass arguments into matlab. So,
                // hopefully this doesn't trip up anything.
                break;
            }
            case matlab::data::ArrayType::ENUM:
            {
                matlab::data::TypedArray<matlab::data::Enumeration> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::SPARSE_LOGICAL:
            {
                matlab::data::SparseArray<bool> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::SPARSE_DOUBLE:
            {
                matlab::data::SparseArray<double> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::SPARSE_COMPLEX_DOUBLE:
            {
                matlab::data::SparseArray<std::complex<double>> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
#ifndef REDUCED_TYPES
            case matlab::data::ArrayType::SPARSE_SINGLE:
            {
                matlab::data::SparseArray<float> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
            case matlab::data::ArrayType::SPARSE_COMPLEX_SINGLE:
            {
                matlab::data::SparseArray<std::complex<float>> tmp = std::move(str[0][field]);
                stmp[0][field] = std::move(tmp);
                break;
            }
#endif // REDUCED_TYPES
            case matlab::data::ArrayType::UNKNOWN:
            {
                utilities::error("addSingleField: unknown array type");
                break;
            }
            }
        }
        stmp[0][fieldname] = value;
        s = std::move(stmp);
    }

    inline void addFieldRecursive(matlab::data::Array &s, std::string fieldname, matlab::data::Array value)
    {
        auto idx = fieldname.find_first_of('.');
        matlab::data::ArrayFactory factory;
        if (idx == std::string::npos)
        {
            if (!utilities::isfield(s, fieldname))
            {
                utilities::addSingleField(s, fieldname, value);
            }
            else
            {
                matlab::data::StructArray str(std::move(s));
                str[0][fieldname] = value;
                s = std::move(str);
            }
        }
        else
        {
            std::string toplevel = fieldname.substr(0, idx);
            std::string nestedlevels = fieldname.substr(idx + 1);
            if (!utilities::isfield(s, toplevel))
            {
                matlab::data::Array nextlevel = factory.createStructArray({1, 1}, {});
                utilities::addSingleField(s, toplevel, nextlevel);
            }
            matlab::data::Array level1 = utilities::getfield(s, toplevel);
            matlab::data::StructArray str(std::move(s));
            utilities::addFieldRecursive(level1, nestedlevels, value);
            str[0][toplevel] = std::move(level1);
            s = std::move(str);
        }
    }

    template<typename T>
    void mapToComplex(const matlab::data::TypedArray<T> &x, matlab::data::TypedArray<std::complex<T>> &y)
    {
        matlab::data::ArrayDimensions dims = x.getDimensions();
        if (dims.size() == 2)
        {
            for (std::size_t iRow = 0; iRow < dims[0]; iRow++)
            {
                for (std::size_t jCol = 0; jCol < dims[1]; jCol++)
                {
                    y[iRow][jCol] = std::complex<T>(x[iRow][jCol], 0.);
                }
            }
        }
        else if (dims.size() == 3)
        {
            for (std::size_t iRow = 0; iRow < dims[0]; iRow++)
            {
                for (std::size_t jCol = 0; jCol < dims[1]; jCol++)
                {
                    for (std::size_t kTen = 0; kTen < dims[2]; kTen++)
                    {
                        y[iRow][jCol][kTen] = std::complex<T>(x[iRow][jCol][kTen], 0.);
                    }
                }
            }
        }
    }
    
    template<typename T>
    matlab::data::TypedArray<std::complex<T>> getascomplex(const matlab::data::Array &x)
    {
        matlab::data::ArrayFactory factory;
        if (matlab::data::ArrayType::COMPLEX_DOUBLE == x.getType())
        {
            return std::move(x);
        }
        else if (matlab::data::ArrayType::DOUBLE == x.getType())
        {
            matlab::data::TypedArray<T> xref(std::move(x));
            matlab::data::TypedArray<std::complex<T>> retval = factory.createArray<std::complex<T>>(xref.getDimensions());
            utilities::mapToComplex(xref, retval);
            return std::move(retval);
        }
        else
        {
            utilities::error("getascomplex: input must be numeric");
        }
        return factory.createArray<std::complex<T>>({0, 0});
    }

    template <typename T>
    T getscalar(const matlab::data::Array &x)
    {
        matlab::data::TypedArray<T> retVal(x);
        return retVal[0];
    }

    inline std::vector<matlab::data::Array> feval(
        const matlab::data::Array &handle,
        const std::size_t numReturned,
        const std::vector<matlab::data::Array> &arguments)
    {
        std::vector<matlab::data::Array> theArgs({handle});
        theArgs.insert(theArgs.end(), arguments.begin(), arguments.end());
        return matlabPtr->feval(
            matlab::engine::convertUTF8StringToUTF16String("feval"),
            static_cast<int>(numReturned),
            theArgs);
    }

#if defined(MATLAB_MEX_FILE)
    inline std::filesystem::path getMexPath()
    {
        using matlab::engine::convertUTF8StringToUTF16String;
        matlab::data::ArrayFactory factory;
        
        std::string arg = "fullpath";
        
        std::vector<matlab::data::Array> args{factory.createScalar(arg)};
        auto retval = feval(factory.createScalar("mfilename"),1, args);
        std::filesystem::path mexPath = getstringvalue(retval[0]);
        return mexPath;
    }
#endif

} // namespace utilities
#endif  // MEX_UTILITIES_HPP