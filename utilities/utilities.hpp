#ifndef MEX_UTILITIES_HPP
#define MEX_UTILITIES_HPP
#include <algorithm>
#include <charconv>
#include <filesystem>
#include <functional>
#include <iterator>
#include <numeric>
#include <optional>
#include <string_view>
#include <vector>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include "MatlabDataArray.hpp"
// ``mex.hpp`` (rather than just ``cppmex/mexMatlabEngine.hpp``) so that on
// matlab releases that ship the ``mex_common_adapter.hpp`` umbrella (R2025b
// onwards), the *inline* bodies of the cppmex ``MATLABEngine::feval``
// overloads -- defined in ``cppmex/detail/mexApiAdapterImpl.hpp`` and pulled
// in transitively from ``mex_common_adapter.hpp`` -- are reachable in every
// TU that consumes ``utilities.hpp``.  Without this, helper TUs of
// multi-file mexes (e.g. ``multifile1.cpp``, which only includes
// ``utilities.hpp``) would leave the R2025b inline ``feval`` ODR-used but
// unreachable, which mingw GCC and ``icx-cl`` surface as an ``undefined
// reference to MATLABEngine::feval(std::string const&, int, ...)`` link
// error.  ``mex.hpp`` does *not* include ``mexFunctionAdapterImpl.hpp``
// (that lives in ``mexAdapter.hpp``), so this is safe to include from every
// helper TU without duplicating the MEX entry-point definitions.
//
// On older matlab releases (e.g. R2022b) ``mex.hpp`` has no umbrella header
// and only the declarations are pulled in -- but that is harmless because
// the cppmex ``MATLABEngine::feval`` overloads on those releases are
// *non-inline* and produce a real exported symbol from the one TU that
// includes ``mexAdapter.hpp`` (the MEX entry-point TU), which the linker
// uses to satisfy the helper-TU references.
#include "mex.hpp"

// matlabPtr is an ``inline`` variable so it has the same identity across every
// TU that includes ``utilities.hpp`` while still being defined exactly once at
// link time.  This replaces the previous ``#ifndef mex_hpp`` extern/definition
// split, which only worked when ``mex.hpp`` was included by exactly one TU.
inline std::shared_ptr<matlab::engine::MATLABEngine> matlabPtr;

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
            std::string("error"),
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
            std::string("warning"),
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
            std::string("fprintf"),
            0,
            std::vector<matlab::data::Array>({factory.createScalar(message)}));
    }

    inline matlab::data::Array getfield(const matlab::data::Array &s, std::string fieldname)
    {
        if (matlab::data::ArrayType::STRUCT != s.getType())
            utilities::errWithId("getfield","input must be a struct");

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
                utilities::errWithId("getfield","field {} not found.", fieldname);
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
                utilities::errWithId("getfield","field {} not found.", fieldname);
            }
        }
        return str;
    }

    namespace details
    {
        // One ``name[i,j,...]`` segment of a nested field specification.
        struct field_segment
        {
            std::string_view name;
            std::vector<std::size_t> subscripts;
        };

        inline std::string_view trim(const std::string_view text)
        {
            const auto first = text.find_first_not_of(" \t");
            if (std::string_view::npos == first)
                return std::string_view{};
            return text.substr(first, text.find_last_not_of(" \t") - first + 1);
        }

        inline field_segment parse_field_segment(const std::string_view segment, const std::string_view field)
        {
            field_segment parsed{segment, {}};

            const auto open = segment.find_first_of('[');
            if (std::string_view::npos == open)
                return parsed;

            const auto close = segment.find_first_of(']', open);
            if (std::string_view::npos == close)
            {
                utilities::error("get_nested: unterminated subscript in {} while processing {}", segment, field);
                return parsed;
            }

            parsed.name = segment.substr(0, open);
            auto subscripts = segment.substr(open + 1, close - open - 1);
            while (true)
            {
                const auto comma = subscripts.find_first_of(',');
                const auto token = details::trim(subscripts.substr(0, comma));
                // ``from_chars`` rather than ``stoul``: ``stoul("1,1")`` happily
                // returns 1 and drops everything from the comma on, which is how
                // matrix subscripts used to be silently truncated to their first
                // dimension.
                std::size_t value{0};
                const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
                if (std::errc() != result.ec || result.ptr != token.data() + token.size())
                {
                    utilities::error("get_nested: {} is not a valid subscript in {} while processing {}", token, segment, field);
                    return parsed;
                }
                parsed.subscripts.push_back(value);
                if (std::string_view::npos == comma)
                    break;
                subscripts.remove_prefix(comma + 1);
            }
            return parsed;
        }

        // Column major (matlab native) linear index of the element addressed by
        // ``subscripts`` in an array of shape ``dims``.
        inline std::size_t to_linear_index(const matlab::data::ArrayDimensions &dims,
                                           const std::vector<std::size_t> &subscripts,
                                           const bool fortranIndex,
                                           const std::string_view name,
                                           const std::string_view field)
        {
            auto zero_based = [&](const std::size_t position) {
                const auto value = subscripts[position];
                if (fortranIndex)
                {
                    if (0 == value)
                        utilities::error("get_nested: subscript {} of {} is 0 but one based indexing was requested while processing {}", position + 1, name, field);
                    return value - 1;
                }
                return value;
            };

            if (subscripts.empty())
                return 0;

            // A lone subscript keeps its historic meaning -- a linear index into
            // the whole struct array -- whatever the shape of that array is.
            if (1 == subscripts.size())
            {
                const auto nElements = std::accumulate(dims.begin(), dims.end(), std::size_t{1}, std::multiplies<std::size_t>());
                const auto index = zero_based(0);
                if (index >= nElements)
                    utilities::error("get_nested: index {} out of bounds on field {} of size {} while processing {}", index, name, dims, field);
                return index;
            }

            // Matlab drops trailing singleton dimensions, so a modelica
            // ``x[i,j,1]`` addressing a matlab ``i x j`` array is legitimate:
            // pad the shape rather than rejecting the extra subscripts.
            if (subscripts.size() < dims.size())
            {
                utilities::error("get_nested: {} subscripts given for field {} of size {} while processing {}", subscripts.size(), name, dims, field);
                return 0;
            }

            std::size_t linear{0};
            std::size_t stride{1};
            for (std::size_t position = 0; position < subscripts.size(); ++position)
            {
                const auto extent = (position < dims.size()) ? dims[position] : std::size_t{1};
                const auto index = zero_based(position);
                if (index >= extent)
                    utilities::error("get_nested: subscript {} of field {} is out of bounds for size {} while processing {}", position + 1, name, dims, field);
                linear += index * stride;
                stride *= extent;
            }
            return linear;
        }

        // Element access that works for struct arrays of any shape.  ``str[i]``
        // only ever supplies a single index, which the matlab data api rejects
        // with "Not enough indices provided" on anything but a vector; iterating
        // is column major and takes the one linear index we have.
        template <typename _StructArray>
        inline matlab::data::ArrayRef element_field(_StructArray &level, const std::size_t element, const std::string &name)
        {
            auto it = level.begin();
            std::advance(it, static_cast<std::ptrdiff_t>(element));
            return (*it)[name];
        }

        template <typename _Scalar>
        inline matlab::data::Array scalar_element(const matlab::data::Array &leaf, const std::size_t linear)
        {
            const matlab::data::TypedArray<_Scalar> typed(leaf);
            matlab::data::ArrayFactory factory;
            return factory.createScalar<_Scalar>(*(typed.cbegin() + static_cast<std::ptrdiff_t>(linear)));
        }

        // Element ``linear`` of ``leaf`` as a 1x1 array of the same type -- the
        // value a trailing subscript on the last segment of a field
        // specification addresses.
        inline matlab::data::Array element_of(const matlab::data::Array &leaf,
                                              const std::size_t linear,
                                              const std::string_view name,
                                              const std::string_view field)
        {
            matlab::data::ArrayFactory factory;
            switch (leaf.getType())
            {
            case matlab::data::ArrayType::DOUBLE:         return details::scalar_element<double>(leaf, linear);
            case matlab::data::ArrayType::SINGLE:         return details::scalar_element<float>(leaf, linear);
            case matlab::data::ArrayType::LOGICAL:        return details::scalar_element<bool>(leaf, linear);
            case matlab::data::ArrayType::INT8:           return details::scalar_element<std::int8_t>(leaf, linear);
            case matlab::data::ArrayType::UINT8:          return details::scalar_element<std::uint8_t>(leaf, linear);
            case matlab::data::ArrayType::INT16:          return details::scalar_element<std::int16_t>(leaf, linear);
            case matlab::data::ArrayType::UINT16:         return details::scalar_element<std::uint16_t>(leaf, linear);
            case matlab::data::ArrayType::INT32:          return details::scalar_element<std::int32_t>(leaf, linear);
            case matlab::data::ArrayType::UINT32:         return details::scalar_element<std::uint32_t>(leaf, linear);
            case matlab::data::ArrayType::INT64:          return details::scalar_element<std::int64_t>(leaf, linear);
            case matlab::data::ArrayType::UINT64:         return details::scalar_element<std::uint64_t>(leaf, linear);
            case matlab::data::ArrayType::COMPLEX_DOUBLE: return details::scalar_element<std::complex<double>>(leaf, linear);
            case matlab::data::ArrayType::COMPLEX_SINGLE: return details::scalar_element<std::complex<float>>(leaf, linear);
            case matlab::data::ArrayType::CHAR:
            {
                const matlab::data::CharArray characters(leaf);
                return factory.createCharArray(std::u16string(1, characters.toUTF16()[linear]));
            }
            case matlab::data::ArrayType::MATLAB_STRING:
            {
                const matlab::data::StringArray strings(leaf);
                matlab::data::StringArray element = factory.createArray<matlab::data::MATLABString>({1, 1});
                element[0] = *(strings.cbegin() + static_cast<std::ptrdiff_t>(linear));
                return element;
            }
            case matlab::data::ArrayType::CELL:
            {
                const matlab::data::CellArray cells(leaf);
                matlab::data::CellArray element = factory.createArray<matlab::data::Array>({1, 1});
                element[0] = *(cells.cbegin() + static_cast<std::ptrdiff_t>(linear));
                return element;
            }
            case matlab::data::ArrayType::STRUCT:
            {
                matlab::data::StructArray structs(leaf);
                auto names = structs.getFieldNames();
                const std::vector<std::string> fieldnames(names.begin(), names.end());
                matlab::data::StructArray element = factory.createStructArray({1, 1}, fieldnames);
                auto selected = *(structs.begin() + static_cast<std::ptrdiff_t>(linear));
                for (const auto &fieldname : fieldnames)
                    element[0][fieldname] = matlab::data::Array(selected[fieldname]);
                return element;
            }
            default:
                utilities::error("get_nested: cannot subscript the {} typed leaf {} while processing {}",
                                 static_cast<int>(leaf.getType()), name, field);
                return leaf;
            }
        }

        // Resolve a nested field specification such as ``a.b[2].c[1,3].d`` against a
        // struct array and hand back a reference to the whole leaf field.  A
        // subscript belongs to the struct array the segment names and selects the
        // element the *following* segment is read from; the subscripts of the last
        // segment are reported through ``leaf`` for the caller to apply to the leaf
        // value.  Subscripts are zero based unless ``fortranIndex`` is set, in which
        // case they follow the one based modelica convention.
        inline matlab::data::ArrayRef walk_nested_field(matlab::data::StructArray& str, const std::string_view field, bool fortranIndex, details::field_segment &leaf) {
            std::vector<std::string_view> segments;
            for (std::string_view remainder = field;;)
            {
                const auto dot = remainder.find_first_of('.');
                segments.push_back(remainder.substr(0, dot));
                if (std::string_view::npos == dot)
                    break;
                remainder.remove_prefix(dot + 1);
            }

            // ``level`` is the struct array the current segment is looked up in --
            // empty for the root ``str`` -- and ``element`` the element of it that
            // the previous segment's subscript selected.  ``std::optional`` because
            // ``StructArrayRef`` has no default constructor: the previous
            // implementation reinterpret_cast'ed one reference onto another to
            // rebind it, which quietly produced an invalid reference (and a matlab
            // crash) whenever a level was not the struct array it was assumed to be.
            std::optional<matlab::data::StructArrayRef> level;
            std::size_t element{0};

            auto lookup = [&](const std::string &name) {
                auto fieldnames = level ? level->getFieldNames() : str.getFieldNames();
                if (std::find(fieldnames.begin(), fieldnames.end(), name) == fieldnames.end())
                    utilities::error("get_nested: invalid field name {} on total field {}", name, field);

                if (0 == (level ? level->getNumberOfElements() : str.getNumberOfElements()))
                    utilities::error("get_nested: field {} is empty while processing {}", name, field);

                return level ? details::element_field(*level, element, name)
                             : details::element_field(str, element, name);
            };

            for (std::size_t i = 0; i + 1 < segments.size(); ++i)
            {
                const auto segment = details::parse_field_segment(segments[i], field);
                const std::string name(segment.name);

                auto value = lookup(name);
                if (matlab::data::ArrayType::STRUCT != value.getType())
                    utilities::error("get_nested: field {} is not a struct while processing {}", name, field);

                level.emplace(value);
                element = details::to_linear_index(level->getDimensions(), segment.subscripts, fortranIndex, name, field);
            }

            leaf = details::parse_field_segment(segments.back(), field);
            return lookup(std::string(leaf.name));
        }
    }

    // Reference to the whole leaf field of a nested field specification.  Any
    // subscript on the last segment is *not* applied -- use
    // ``get_nested_field`` for that -- because a single element of a numeric
    // leaf has no ``ArrayRef`` representation to alias.
    // ``inline`` rather than ``static``: with internal linkage every consuming
    // TU gets its own copy, and any TU that does not call it trips
    // "unreferenced function with internal linkage" (MSVC C4505) or
    // -Wunused-function.
    inline matlab::data::ArrayRef get_nested_field_ref(matlab::data::StructArray& str, const std::string_view field, bool fortranIndex = false) {
        details::field_segment leaf;
        return details::walk_nested_field(str, field, fortranIndex, leaf);
    }

    // Value addressed by a nested field specification such as
    // ``a.b[2].c[1,3].d`` or ``boat.boardP.liftingLine[1,2].f_flap[3,1]``.  A
    // subscript on the last segment selects that single element of the leaf and
    // is returned as a 1x1 array of the leaf's type.
    inline matlab::data::Array get_nested_field(matlab::data::StructArray& str, const std::string_view field, bool fortranIndex = false) {
        details::field_segment leaf;
        matlab::data::Array value = details::walk_nested_field(str, field, fortranIndex, leaf);

        if (leaf.subscripts.empty())
            return value;

        const auto linear = details::to_linear_index(value.getDimensions(), leaf.subscripts, fortranIndex, leaf.name, field);
        return details::element_of(value, linear, leaf.name, field);
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
            std::string("feval"),
            static_cast<int>(numReturned),
            theArgs);
    }

#if defined(MATLAB_MEX_FILE)
    inline std::filesystem::path getMexPath()
    {
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