#include "mex.hpp"
#include "mexAdapter.hpp"
#include "utilities.hpp"

class MexFunction
    : public matlab::mex::Function
{

public:
    MexFunction()
    {
        matlabPtr = getEngine();
    }
    ~MexFunction() = default;
    void operator()([[maybe_unused]] matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        if (inputs.size())
        {
            matlab::data::ArrayFactory f;
            matlab::data::StructArray retval = f.createStructArray({1, 1}, {"hello","foo","bar"});
            matlab::data::StructArray field = f.createStructArray({1, 1}, {"world"});
            matlab::data::StructArray foo = f.createStructArray({2, 1}, {"bar"});
            matlab::data::StructArray bar = f.createStructArray({2, 1}, {"baz"});
            matlab::data::StructArray baz = f.createStructArray({2, 1}, {"boom"});

            matlab::data::StructArray str = std::move(inputs[0]);

            auto worldref = utilities::get_nested_field(str, "hello.world");
            auto foo0barref = utilities::get_nested_field(str, "foo[0].bar");
            auto foo1barref = utilities::get_nested_field(str, "foo[1].bar");
            auto boom0ref = utilities::get_nested_field(str, "bar.baz[0].boom");
            auto boom1ref = utilities::get_nested_field(str, "bar.baz[1].boom");
            field[0]["world"] = matlab::data::Array(worldref);
            retval[0]["hello"] = std::move(field);
            foo[0]["bar"] = matlab::data::Array(foo0barref);
            foo[1]["bar"] = matlab::data::Array(foo1barref);
            bar[0]["baz"] = matlab::data::Array(boom0ref);
            bar[1]["baz"] = matlab::data::Array(boom1ref);
            retval[0]["foo"] = std::move(foo);
            retval[0]["bar"] = std::move(bar);
            outputs[0] = std::move(retval);
        }
        else
        {
            utilities::error("Call with struct('hello',struct('world',<value>))\n");
        }
    }
};