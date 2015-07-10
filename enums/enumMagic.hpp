/*
Copyright (C) 2012 Christoph Husse

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

/*
    Visual Studio 2010 needs explicit macro expansion when passing VA_ARGS
    to a VA_ARGS macro. But other compilers require this expansion to be skipped entirely.
*/
#ifdef _MSC_VER
    // the Intel Compiler doesn't need expansion and even worse, it fails when enabling it...
    #ifndef __INTEL_COMPILER
        #if (_MSC_VER < 1600)
            #error "The Microsoft C++ compiler in Visual Studio 2008 and lower is probably not supported (even if you replace all 'auto' keywords)."
        #else
            #define PP_MACRO_COMPILER_EXPAND_WORKAROUND
        #endif
    #endif
#else
    // other compilers also seem to work without it... Verified: CLang 3, GCC 4.6
#endif

/********************** COUNT_VA_ARGS 
    It feels wierd that at every corner you are told you can not count VA_ARGS and boost even requires
    you to give the explicit number. That's not necessary, since here is the way how you can count them,
    which is quite easy... 
*/
#ifdef PP_MACRO_COMPILER_EXPAND_WORKAROUND
    #define COUNT_VA_ARGS(...) BOOST_PP_EXPAND(__COUNT_VA_ARGS (__VA_ARGS__, 63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,\
        42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1))
#else
    #define COUNT_VA_ARGS(...) __COUNT_VA_ARGS (__VA_ARGS__, 63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,\
        42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1)
#endif

#define __COUNT_VA_ARGS(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,\
    _29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,\
    _60,_61,_62,_63,N,...) N

/********************** PP_MACRO_FOREACH 
    # Description:
        If one of the backbones of most preprocessor metaprogramming I do.
        It iterates through a list of N 2-tuples and expands a hook macro
        N times with every each of those 2-tuples as argument. This is a
        simple way to generate arbitrary code based on N parameters.

    # Arguments:
        <Namespace>        Namespace in which to place the new enumeration (supporting nested namespaces like "LevelA::LevelB")
        <Name>            Name of the new enumeration, like "EShaderType"
        <...>            A list of enumeration members like "a,b,c,d,e,"
                        Place no comma after the last parameter and only write valid C identifiers.
*/
#define __PP_MACRO_FOREACH_EXEC(_, Hook, Elem)        Hook(BOOST_PP_TUPLE_ELEM(2,0,Elem), BOOST_PP_TUPLE_ELEM(2,1,Elem))
#define PP_MACRO_FOREACH(N,Hook,Tuple_2xn)            BOOST_PP_LIST_FOR_EACH(__PP_MACRO_FOREACH_EXEC, Hook, BOOST_PP_TUPLE_TO_LIST(N,Tuple_2xn))

/********************** PP_MACRO_FOREACH_I
    # Description:
        Pretty much the same as PP_MACRO_FOREACH, but uses a "for(int i = 0; i < N; i++)"-like 
        loop and passes the loop variable "i" as additional first parameter to the specified 
        hook.

    # Arguments:
        <Namespace>        Namespace in which to place the new enumeration (supporting nested namespaces like "LevelA::LevelB")
        <Name>            Name of the new enumeration, like "EShaderType"
        <...>            A list of enumeration members like "a,b,c,d,e,"
                        Place no comma after the last parameter and only write valid C identifiers.
*/
#define __PP_MACRO_FOREACH_I_EXEC(_, Hook, i, Elem)        Hook(i, BOOST_PP_TUPLE_ELEM(2,0,Elem), BOOST_PP_TUPLE_ELEM(2,1,Elem))
#define PP_MACRO_FOREACH_I(N,Hook,Tuple_2xn)            BOOST_PP_LIST_FOR_EACH_I(__PP_MACRO_FOREACH_EXEC, Hook, BOOST_PP_TUPLE_TO_LIST(N,Tuple_2xn))



#if !CFLAGS_ENABLE_ENUM_CLASS_WORKAROUND
    #define __PP_MACRO_ENUM_WORKAROUND_LINKER(Namespace, TEnum, N, Tuple_Pairs)
#else
    // satisfy the linker in case somehow the raw enum type gets used as template parameter
    #define __PP_MACRO_ENUM_WORKAROUND_LINKER(Namespace, TEnum, N, Tuple_Pairs) \
        __PP_MACRO_ENUM_LINKER_(Namespace, TEnum::TValue, TEnum##_TValue, N, Tuple_Pairs)
#endif

#define PP_MACRO_NAMESPACE_FOLD(_, state, x) ::x
#define PP_MACRO_NAMESPACE(list) BOOST_PP_SEQ_FOR_EACH(PP_MACRO_NAMESPACE_FOLD, _, list)

#define PP_MACRO_NAMESPACE_BEGIN_FOLD(_, state, x) namespace x {
#define PP_MACRO_NAMESPACE_BEGIN(list) BOOST_PP_SEQ_FOR_EACH(PP_MACRO_NAMESPACE_BEGIN_FOLD, _, list)

#define PP_MACRO_NAMESPACE_END_FOLD(_, state, _1) }
#define PP_MACRO_NAMESPACE_END(list) BOOST_PP_SEQ_FOR_EACH(PP_MACRO_NAMESPACE_END_FOLD, _, list)

#define PP_MACRO_METAENUM_NAMESPACE_NAME        PP_MACRO_NAMESPACE(PP_MACRO_METAENUM_NAMESPACE)::Internal::MetaEnumerations
#define PP_MACRO_METAENUM_NAMESPACE_DEFBEGIN    PP_MACRO_NAMESPACE_BEGIN(PP_MACRO_METAENUM_NAMESPACE) namespace Internal { namespace MetaEnumerations {
#define PP_MACRO_METAENUM_NAMESPACE_DEFEND        PP_MACRO_NAMESPACE_END(PP_MACRO_METAENUM_NAMESPACE) }}

/********************** PP_MACRO_ENUM_CLASS_EX
    # Description:
        Creates an enumeration type with custom enumeration values. This is the most flexible version
        but requires a little more verbosity. See PP_MACRO_ENUM_CLASS() for a straighforward version.

        PP_MACRO_ENUM_CLASS_EX(MyNamespace, MyEnum, (a,3),(b,4),(c,9),(d,-11)) creates an enumeration like

            namspace MyNamspace { enum MyEnum{ a = 3, b = 4, c = 9, d = -11 }; }

    # Arguments:
        <Namespace>        Namespace in which to place the new enumeration (supporting nested namespaces like "LevelA::LevelB")
        <Name>            Name of the new enumeration, like "EShaderType"
        <...>            A list of tuples like "(a,3),(b,4),(c,9),(d,-11)"
                        Place no comma after the last parameter and only write valid C identifiers
                        for the first tuple members and valid integers for the second.

    # Remarks:
        This macro looks more complicated than it is. One reason is the support of setting
        an enum value based (even computationally) on previously declared enum members. 
*/
#define __PP_MACRO_ENUM_MEMBER(a,b)                    a = b,
#define __PP_MACRO_ENUM_MEMBER_PAIR(_, NamespaceName, Elem)\
    (std::make_pair(BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(2,0,Elem)), \
        (int)(BOOST_PP_TUPLE_ELEM(2,0,NamespaceName)::BOOST_PP_TUPLE_ELEM(2,1,NamespaceName)::BOOST_PP_TUPLE_ELEM(2,0,Elem))))
#define __PP_MACRO_ENUM_MEMBER_PAIR_IMP(_, NamespaceName, Elem)\
    (std::make_pair(BOOST_PP_STRINGIZE(Elem), (int)(BOOST_PP_TUPLE_ELEM(2,0,NamespaceName)::BOOST_PP_TUPLE_ELEM(2,1,NamespaceName)::Elem)))


#define PP_MACRO_ENUM_CLASS_EX(Namespace, Name, ...)\
    PP_MACRO_ENUM_CLASS_N (Namespace, Name, COUNT_VA_ARGS (__VA_ARGS__), (__VA_ARGS__))

#define PP_MACRO_ENUM_CLASS_N(Namespace, Name, N, Tuple_2xn)\
    PP_MACRO_ENUM_DEFINITION(Namespace, Name, N,\
        PP_MACRO_ENUM_DECLARATION(Namespace, Name, BOOST_PP_TUPLE_ELEM(2,0,BOOST_PP_TUPLE_ELEM(N,0,Tuple_2xn)), PP_MACRO_FOREACH(N, __PP_MACRO_ENUM_MEMBER, Tuple_2xn)),\
        BOOST_PP_LIST_FOR_EACH(__PP_MACRO_ENUM_MEMBER_PAIR, (PP_MACRO_NAMESPACE(Namespace), Name), BOOST_PP_TUPLE_TO_LIST(N,Tuple_2xn)))



/********************** PP_MACRO_ENUM_CLASS 
    # Description:
        Is a shortcut for PP_MACRO_ENUM_CLASS_EX() and instead of passing a list of tuples, you
        just pass a list of identifiers. The macro automatically create a list of tuples with
        the default enumeration values ranging from 0,...,N-1 where "0" is assigned to the first
        identifier, "1" to the second and so forth. For example:

        PP_MACRO_ENUM_CLASS(MyNamespace, MyEnum, a,b,c,d) create an enumeration like

            namspace MyNamspace { enum MyEnum{ a = 0, b = 1, c = 2, d = 3 }; }

    # Arguments:
        <Namespace>        Namespace in which to place the new enumeration (supporting nested namespaces like "LevelA::LevelB")
        <Name>            Name of the new enumeration, like "EShaderType"
        <...>            A list of enumeration members like "a,b,c,d,e"
                        Place no comma after the last parameter and only write valid C identifiers.
*/
#define __PP_MACRO_ENUM_CLASS_ITEM(r, N, i, Elem)        (Elem,i) BOOST_PP_COMMA_IF(BOOST_PP_SUB(BOOST_PP_SUB(N,1),i))

#ifdef PP_MACRO_COMPILER_EXPAND_WORKAROUND
    #define PP_MACRO_ENUM_CLASS(Namespace, Name, ...)        BOOST_PP_EXPAND(PP_MACRO_ENUM_CLASS_EX (Namespace, Name,\
        BOOST_PP_LIST_FOR_EACH_I(__PP_MACRO_ENUM_CLASS_ITEM, COUNT_VA_ARGS(__VA_ARGS__), BOOST_PP_TUPLE_TO_LIST(COUNT_VA_ARGS(__VA_ARGS__), (__VA_ARGS__)))))
#else
    #define PP_MACRO_ENUM_CLASS(Namespace, Name, ...)        PP_MACRO_ENUM_CLASS_EX (Namespace, Name,\
        BOOST_PP_LIST_FOR_EACH_I(__PP_MACRO_ENUM_CLASS_ITEM, COUNT_VA_ARGS(__VA_ARGS__), BOOST_PP_TUPLE_TO_LIST(COUNT_VA_ARGS(__VA_ARGS__), (__VA_ARGS__))))
#endif

#define PP_MACRO_NOTHING

#define PP_MACRO_ENUM_CLASS_IMP(Namespace, Name, ...)        PP_MACRO_ENUM_DEFINITION(Namespace, Name, COUNT_VA_ARGS(__VA_ARGS__), PP_MACRO_NOTHING,\
        BOOST_PP_LIST_FOR_EACH(__PP_MACRO_ENUM_MEMBER_PAIR_IMP, (PP_MACRO_NAMESPACE(Namespace), Name), BOOST_PP_TUPLE_TO_LIST(COUNT_VA_ARGS(__VA_ARGS__),(__VA_ARGS__))))


#define __PP_MACRO_ENUM_LINKER_(Namespace, TEnum, TEnumName, N, Tuple_Pairs) \
    BOOST_PP_EXPAND(__PP_MACRO_ENUM_LINKER_SUB(Namespace, TEnum, BOOST_PP_CAT(_, BOOST_PP_CAT(__COUNTER__, BOOST_PP_CAT(_, TEnumName))), Tuple_Pairs))
    //template<> PP_MACRO_STD_UNORDERED_MAP<PP_MACRO_STD_STRING, int, PP_MACRO_STD_IHASH, PP_MACRO_STD_IEQUAL_TO> EnumSupport<PP_MACRO_NAMESPACE(Namespace)::TEnum>::m_StringToInt = 
    //    PP_MACRO_STD_UNORDERED_MAP<PP_MACRO_STD_STRING, int, PP_MACRO_STD_IHASH, PP_MACRO_STD_IEQUAL_TO>();
    //template<> PP_MACRO_STD_UNORDERED_MAP<int, PP_MACRO_STD_STRING> EnumSupport<PP_MACRO_NAMESPACE(Namespace)::TEnum>::m_IntToString =
    //    PP_MACRO_STD_UNORDERED_MAP<int, PP_MACRO_STD_STRING>();
    //template<> PP_MACRO_STD_VECTOR<PP_MACRO_STD_STRING> EnumSupport<PP_MACRO_NAMESPACE(Namespace)::TEnum>::m_Names = 
    //    PP_MACRO_STD_VECTOR<PP_MACRO_STD_STRING>();
    //template<> PP_MACRO_STD_VECTOR<PP_MACRO_NAMESPACE(Namespace)::TEnum> EnumSupport<PP_MACRO_NAMESPACE(Namespace)::TEnum>::m_Values = 
    //    PP_MACRO_STD_VECTOR<PP_MACRO_NAMESPACE(Namespace)::TEnum>();
    //template<> int EnumSupport<PP_MACRO_NAMESPACE(Namespace)::TEnum>::m_AllFlags = 0;
    //BOOST_PP_EXPAND(__PP_MACRO_ENUM_LINKER_SUB(Namespace, TEnum, BOOST_PP_CAT(_, BOOST_PP_CAT(__COUNTER__, BOOST_PP_CAT(_, TEnumName))), Tuple_Pairs))



//#define __PP_MACRO_ENUM_LINKER_(Namespace, TEnum, TEnumName, N, Tuple_Pairs)
    //template<> PP_MACRO_STD_UNORDERED_MAP<PP_MACRO_STD_STRING, int, PP_MACRO_STD_IHASH, PP_MACRO_STD_IEQUAL_TO> EnumSupport<PP_MACRO_NAMESPACE(Namespace)::TEnum>::m_StringToInt = 
    //    PP_MACRO_STD_UNORDERED_MAP<PP_MACRO_STD_STRING, int, PP_MACRO_STD_IHASH, PP_MACRO_STD_IEQUAL_TO>();
    //template<> PP_MACRO_STD_UNORDERED_MAP<int, PP_MACRO_STD_STRING> EnumSupport<PP_MACRO_NAMESPACE(Namespace)::TEnum>::m_IntToString =
    //    PP_MACRO_STD_UNORDERED_MAP<int, PP_MACRO_STD_STRING>();
    //template<> PP_MACRO_STD_VECTOR<PP_MACRO_STD_STRING> EnumSupport<PP_MACRO_NAMESPACE(Namespace)::TEnum>::m_Names = 
    //    PP_MACRO_STD_VECTOR<PP_MACRO_STD_STRING>();
    //template<> PP_MACRO_STD_VECTOR<PP_MACRO_NAMESPACE(Namespace)::TEnum> EnumSupport<PP_MACRO_NAMESPACE(Namespace)::TEnum>::m_Values = 
    //    PP_MACRO_STD_VECTOR<PP_MACRO_NAMESPACE(Namespace)::TEnum>();
    //template<> int EnumSupport<PP_MACRO_NAMESPACE(Namespace)::TEnum>::m_AllFlags = 0;
    //BOOST_PP_EXPAND(__PP_MACRO_ENUM_LINKER_SUB(Namespace, TEnum, BOOST_PP_CAT(_, BOOST_PP_CAT(__COUNTER__, BOOST_PP_CAT(_, TEnumName))), Tuple_Pairs))

#define __PP_MACRO_ENUM_LINKER_SUB(Namespace, TEnum, Name, Tuple_Pairs)\
    namespace{\
        static struct BOOST_PP_CAT(INIT_, Name)\
        {\
            BOOST_PP_CAT(INIT_, Name)() { \
                PP_MACRO_METAENUM_NAMESPACE_NAME::EnumSupport<PP_MACRO_NAMESPACE(Namespace)::TEnum>::Create(boost::assign::list_of Tuple_Pairs);\
            }\
        }BOOST_PP_CAT(g_Init_, Name);\
    }


#define PP_MACRO_ENUM_DEFINITION(Namespace, TEnum, N, Tuple_2xn, Tuple_Pairs)\
    Tuple_2xn\
    PP_MACRO_METAENUM_NAMESPACE_DEFBEGIN \
        __PP_MACRO_ENUM_LINKER_(Namespace, TEnum, TEnum, N, Tuple_Pairs)\
        __PP_MACRO_ENUM_WORKAROUND_LINKER(Namespace, TEnum, N, Tuple_Pairs)\
    PP_MACRO_METAENUM_NAMESPACE_DEFEND


