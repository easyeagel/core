
#include<iostream>

#include"enums.hpp"
#include"enumTest.hpp" 

    // consider the following to be your existing enum
    namespace Some { namespace Nested { namespace Namespace {
        struct Struct
        {

        enum class ESomeEnum
        {
            a_0 = 1, b_1 = 4, c_2 = 8, d_3 = 3,
        };
        };
    }}}

    // now it is simple to import it:
GMacroEnumImport(
    (Some)(Nested)(Namespace)(Struct),
    ESomeEnum,
    a_0, b_1, c_2, d_3
)


int main()
{
    std::cout << meta::enums::toString(Some::Nested::Namespace::Struct::ESomeEnum::a_0) << std::endl;
    return 0;
}

