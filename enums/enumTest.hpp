
#include"enums.hpp"

GMacroEnumEx(
    (DemoEnum)(SubNamespace), // put in namespace "DemoEnum::SubNamespace"
    EType_B, // typename
    (e_0, 2), 
    (f_1, 8), 
    (g_2, 1), 
    (all, e_0 | f_1 | g_2), 
    (alias, f_1)
);

