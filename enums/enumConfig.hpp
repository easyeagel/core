#pragma once

/*    You can use different implementations for common STD types by simply
    defining the following macros appropriately before including this file.
*/

//#define PP_MACRO_STD_VECTOR std::vector
//#define PP_MACRO_STD_STRING std::string
//#define PP_MACRO_STD_UNORDERED_MAP boost::unordered_map
//#define PP_MACRO_STD_IEQUAL_TO hash_examples::iequal_to
//#define PP_MACRO_STD_IHASH hash_examples::ihash
//#define PP_MACRO_ENUM_ARG_EXCEPTION std::exception
//#define PP_MACRO_ENUM_ARG_EXCEPTION_RAISE(details) throw PP_MACRO_ENUM_ARG_EXCEPTION()
//#define PP_MACRO_STD_MAKE_PAIR std::make_pair
//#define PP_MACRO_METAENUM_NAMESPACE (MetaEnumerations)
//#define PP_MACRO_METAENUM_ENUM_NAMESPACE (MetaEnumerations)(Enum)
//#define PP_MACRO_METAENUM_FLAGS_NAMESPACE (MetaEnumerations)(Flags)


#ifndef PP_MACRO_STD_VECTOR
    #define PP_MACRO_STD_VECTOR std::vector
#endif

#ifndef PP_MACRO_STD_STRING
    #define PP_MACRO_STD_STRING std::string
#endif

#ifndef PP_MACRO_STD_UNORDERED_MAP
    #define PP_MACRO_STD_UNORDERED_MAP boost::unordered_map
#endif

#ifndef PP_MACRO_STD_IEQUAL_TO
    #define PP_MACRO_STD_IEQUAL_TO hash_examples::iequal_to
#endif

#ifndef PP_MACRO_STD_IHASH
    #define PP_MACRO_STD_IHASH hash_examples::ihash
#endif

#ifndef PP_MACRO_ENUM_ARG_EXCEPTION
    #define PP_MACRO_ENUM_ARG_EXCEPTION std::exception
#endif

#ifndef PP_MACRO_ENUM_ARG_EXCEPTION_RAISE
    #define PP_MACRO_ENUM_ARG_EXCEPTION_RAISE(details) throw PP_MACRO_ENUM_ARG_EXCEPTION()
#endif

#ifndef PP_MACRO_STD_MAKE_PAIR
    #define PP_MACRO_STD_MAKE_PAIR std::make_pair
#endif

/*    Where to put internal support classes and namespaces? 
    If you want nested namespaces like LevelA::LevelB::LevelC you have to
    write (LevelA)(LevelB)(LevelC) instead!

    Usually you want to put them somewhere where they don't interfer
    with your own namespaces. You don't need to access them manually.

    NOTE: The namespace you choose here is prepended to "::Internal::MetaEnumerations"
    so you can safely reuse the same namespace for other things.
*/
#ifndef PP_MACRO_METAENUM_NAMESPACE
    #define PP_MACRO_METAENUM_NAMESPACE (MetaEnumerations)
#endif

// where to put Enum::ToString() and stuff like that
#ifndef PP_MACRO_METAENUM_ENUM_NAMESPACE
    #define PP_MACRO_METAENUM_ENUM_NAMESPACE (MetaEnumerations)(Enum)
#endif

// where to put Flags::ToString() and stuff like that
#ifndef PP_MACRO_METAENUM_FLAGS_NAMESPACE
    #define PP_MACRO_METAENUM_FLAGS_NAMESPACE (MetaEnumerations)(Flags)
#endif

/*
    Enum classes are a C++0x feature. If the compiler supports it and it is not
    overriden, we enable it. Otherwise, a derivative of the "enum hack" is used
    to emulate the behaviour of enum classes in usual C++...

    You can set CFLAGS_ENABLE_ENUM_CLASS_WORKAROUND to "0" to force the use of
    enum classes or "1" to force the "enum hack".
*/
#ifndef CFLAGS_ENABLE_ENUM_CLASS_WORKAROUND
    #ifdef _MSC_VER
        // the Intel Compiler doesn't need expansion and even worse, it fails when enabling it...
        #ifndef __INTEL_COMPILER
            #if (_MSC_VER < 1700) // supported only by Visual Studio 2011 and higher
                #define CFLAGS_ENABLE_ENUM_CLASS_WORKAROUND 1
            #endif
        #else
            #if __cplusplus <= 199711L // does not support C++11?
                #define CFLAGS_ENABLE_ENUM_CLASS_WORKAROUND 1
            #endif
        #endif
    #else
        #if __cplusplus <= 199711L // does not support C++11?
            #define CFLAGS_ENABLE_ENUM_CLASS_WORKAROUND 1
        #endif
    #endif
#endif
