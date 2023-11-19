#pragma once
#include <fmt/core.h>
#include "MatlabDataArray.hpp"

std::shared_ptr<matlab::engine::MATLABEngine> matlabPtr;

namespace utilities
{

    inline bool isstruct(const matlab::data::Array &x)
    {
        return (matlab::data::ArrayType::STRUCT == x.getType());
    }

    inline bool isfield(const matlab::data::Array &s, std::string fieldname)
    {
        if (matlab::data::ArrayType::STRUCT != s.getType())
            return false;
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
                return utilities::isfield(str[0][toplevel], nestedlevels);
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
            matlab::data::CharArray retVal(std::move(x));
            return retVal.toAscii();
        }
        else if (matlab::data::ArrayType::MATLAB_STRING == x.getType())
        {
            matlab::data::StringArray retVal(std::move(x));
            return retVal[0];
        }
        return std::string("");
    }

    template <typename... _Args>
    void error(fmt::v10::format_string<_Args...> __fmt, _Args &&...__args)
    {
        matlab::data::ArrayFactory factory;
        std::string message = fmt::format(__fmt, __args...);
        matlabPtr->feval(
            matlab::engine::convertUTF8StringToUTF16String("error"),
            0,
            std::vector<matlab::data::Array>({factory.createScalar(message)}));
    }

    template <typename... _Args>
    void printf(fmt::v10::format_string<_Args...> __fmt, _Args &&...__args)
    {
        matlab::data::ArrayFactory factory;
        std::string message = fmt::format(__fmt, __args...);
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
                utilities::error(std::string("getfield: " + fieldname + " not found"));
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
                utilities::error(std::string("getfield: " + fieldname + " not found"));
            }
        }
        return str;
    }

    inline void addSingleField(matlab::data::Array &s, std::string fieldname, const matlab::data::Array value)
    {
        matlab::data::ArrayFactory factory;
        matlab::data::StructArray str(std::move(s));
        std::vector<std::string> fieldnames(str.getFieldNames().begin(), str.getFieldNames().end());
        fieldnames.push_back(fieldname);
        matlab::data::StructArray stmp = factory.createStructArray({1, 1}, fieldnames);
        for (auto field : str.getFieldNames())
        {
            stmp[0][field] = std::move(str[0][field]);
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

    inline void mapToComplex(const matlab::data::TypedArray<double> &x, matlab::data::TypedArray<std::complex<double>> &y)
    {
        auto dims = x.getDimensions();
        if (dims.size() == 2)
        {
            for (auto iRow = 0; iRow < dims[0]; iRow++)
            {
                for (auto jCol = 0; jCol < dims[1]; jCol++)
                {
                    y[iRow][jCol] = std::complex<double>(x[iRow][jCol], 0.);
                }
            }
        }
        else if (dims.size() == 3)
        {
            for (auto iRow = 0; iRow < dims[0]; iRow++)
            {
                for (auto jCol = 0; jCol < dims[1]; jCol++)
                {
                    for (auto kTen = 0; kTen < dims[2]; kTen++)
                    {
                        y[iRow][jCol][kTen] = std::complex<double>(x[iRow][jCol][kTen], 0.);
                    }
                }
            }
        }
    }

    inline matlab::data::TypedArray<std::complex<double>> getascomplex(const matlab::data::Array &x)
    {
        matlab::data::ArrayFactory factory;
        if (matlab::data::ArrayType::COMPLEX_DOUBLE == x.getType())
        {
            return std::move(x);
        }
        else if (matlab::data::ArrayType::DOUBLE == x.getType())
        {
            matlab::data::TypedArray<double> xref(std::move(x));
            matlab::data::TypedArray<std::complex<double>> retval = factory.createArray<std::complex<double>>(xref.getDimensions());
            utilities::mapToComplex(xref, retval);
            return std::move(retval);
        }
        else
        {
            utilities::error("getascomplex: input must be numeric");
        }
        return factory.createArray<std::complex<double>>({0, 0});
    }

    template <typename T>
    T getscalar(const matlab::data::Array &x)
    {
        matlab::data::TypedArray<T> retVal(std::move(x));
        return retVal[0];
    }

} // namespace utilites
