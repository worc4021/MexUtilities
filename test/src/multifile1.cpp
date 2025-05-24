#include "multifile.hpp"
#include "utilities.hpp"

int TestStruct::sum(void) const
{
    if (1 == a)
        utilities::printf("First argument is 1.");
    return a + b;
}