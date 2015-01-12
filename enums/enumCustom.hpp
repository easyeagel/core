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

PP_MACRO_METAENUM_NAMESPACE_DEFBEGIN
    template<class TEnum>
    struct EnumSupport
    {
    private:
        static PP_MACRO_STD_UNORDERED_MAP<PP_MACRO_STD_STRING, int, PP_MACRO_STD_IHASH, PP_MACRO_STD_IEQUAL_TO>& m_StringToInt()
        {
            static PP_MACRO_STD_UNORDERED_MAP<PP_MACRO_STD_STRING, int, PP_MACRO_STD_IHASH, PP_MACRO_STD_IEQUAL_TO> gs;
            return gs;
        }

        static PP_MACRO_STD_UNORDERED_MAP<int, PP_MACRO_STD_STRING>& m_IntToString()
        {
            static PP_MACRO_STD_UNORDERED_MAP<int, PP_MACRO_STD_STRING> gs;
            return gs;
        }

        static PP_MACRO_STD_VECTOR<PP_MACRO_STD_STRING>& m_Names()
        {
            static PP_MACRO_STD_VECTOR<PP_MACRO_STD_STRING> gs;
            return gs;
        }

        static PP_MACRO_STD_VECTOR<TEnum>& m_Values()
        {
            static PP_MACRO_STD_VECTOR<TEnum> gs;
            return gs;
        }

        static int& m_AllFlags()
        {
            static int gs;
            return gs;
        }

        static void CheckInit()
        {
#ifdef _DEBUG
            if(m_Values().size() == 0)
            {
                assert(NULL == std::cout << std::endl << "[ASSERTION FAILED]: Enumeration \"" << typeid(TEnum).name() <<
                    "\" was not properly created to work with this method. (See macro PP_MACRO_ENUM_CLASS[_EX])" << std::endl); 
            }
#endif
        }

    public:
        /*
            The following is called during static initialization and sets everything up.
        */
        template<class TInit>
        static void Create(TInit inStringToInt)
        {
            static bool inited=false;
            if(inited)
                return;
            inited=true;

            assert(m_Values().size() == 0);

            m_AllFlags() = 0;

            BOOST_FOREACH(auto e, inStringToInt) {
                m_Values().push_back((TEnum)e.second);
                m_Names().push_back(e.first);
                m_StringToInt().insert(e);

                // check for flag
                bool isFlag = false;

                for(int i = 0; i < 32; i++)
                {
                    if( ((1 << i) & (int)e.second) == (int)e.second )
                    {
                        isFlag = true;
                        break;
                    }
                }

                if(isFlag)
                    m_AllFlags() |= (int)e.second;
            }

            BOOST_FOREACH(auto e, m_StringToInt())
            {
                if(m_IntToString().find(e.second) != m_IntToString().end())
                    continue;

                m_IntToString().insert(PP_MACRO_STD_MAKE_PAIR(e.second, e.first));
            }
        }

        static bool TryEnumToString(TEnum inEnum, PP_MACRO_STD_STRING* outString)\
        {
            CheckInit();

            auto res = m_IntToString().find((int)inEnum);
    
            if(outString != NULL)
                *outString = PP_MACRO_STD_STRING();
    
            if(res == m_IntToString().end())
                return false;
    
            if(outString != NULL)
                *outString = res->second;
    
            return true;
        }
    
        static bool TryStringToEnum(PP_MACRO_STD_STRING inString, TEnum* outEnum)
        {
            CheckInit();

            auto res = m_StringToInt().find(inString);
    
            if(res == m_StringToInt().end())
                return false;
    
            if(outEnum != NULL)
                *outEnum = (TEnum)res->second;
    
            return true;
        }

        static bool IsValidEnum(int inProbableEnum) { 
            CheckInit();

            return TryEnumToString((TEnum)inProbableEnum, NULL);
        }

        static PP_MACRO_STD_VECTOR<TEnum> GetEnumValues() {
            CheckInit();

            return m_Values(); 
        }

        static PP_MACRO_STD_VECTOR<PP_MACRO_STD_STRING> GetEnumNames() { 
            CheckInit();
            
            return m_Names(); 
        }

        static PP_MACRO_STD_STRING EnumToString(TEnum inEnum) {         
            PP_MACRO_STD_STRING res = "";

            CheckInit();
        
            if(!TryEnumToString(inEnum, &res))
                PP_MACRO_ENUM_ARG_EXCEPTION_RAISE("Given enumeration value is not valid!");
        
            return res;
        }

        static TEnum StringToEnum(PP_MACRO_STD_STRING inString)
        {
            TEnum res;

            CheckInit();
    
            if(!TryStringToEnum(inString, &res))
                PP_MACRO_ENUM_ARG_EXCEPTION_RAISE(("Given enumeration value \"" + inString + "\" is not valid!").c_str());
    
            return res;
        }

        static TEnum IntegerToEnum(int inProbableEnum)
        {
            CheckInit();

            if(!TryEnumToString((TEnum)inProbableEnum, NULL))
                PP_MACRO_ENUM_ARG_EXCEPTION_RAISE("Given enumeration value is not valid!");
    
            return (TEnum)inProbableEnum;
        }

        static bool AreValidFlags(int inProbableEnum) { return (m_AllFlags() | inProbableEnum) == m_AllFlags(); }
        static bool IsFlagSet(TEnum inFlags, TEnum inFlagToCheck)  { return (((int)inFlags) & ((int)inFlagToCheck)) != 0; }
        static bool IsAnyFlagSet(TEnum inFlags, TEnum inFlagsToCheck) { return (((int)inFlags) & ((int)inFlagsToCheck)) != 0; }
        static bool AreAllFlagsSet(TEnum inFlags, TEnum inFlagsToCheck) { return (((int)inFlags) & ((int)inFlagsToCheck)) == ((int)inFlags); }

        static PP_MACRO_STD_VECTOR<TEnum> DecomposeFlags(TEnum inFlags) { 
            PP_MACRO_STD_VECTOR<TEnum> res;

            for(int i = 0; i < 32; i++)
            {
                int value = ((1 << i) & (int)inFlags);

                if(value != 0)
                {
                    if(!IsValidEnum(value))
                        PP_MACRO_ENUM_ARG_EXCEPTION_RAISE("Given flags value is not valid!");

                    res.push_back((TEnum)value);
                }
            }

            return res; 
        }

        static bool TryIntegerToFlags(int inProbableEnum, TEnum* outValue) { 

            int res = 0;

            for(int i = 0; i < 32; i++)
            {
                int value = ((1 << i) & inProbableEnum);

                if((value != 0) && !IsValidEnum(value))
                    return false;

                res |= (int)value;
            }

            if(outValue != NULL)
                *outValue = (TEnum)res;

            return true;
        }

        static PP_MACRO_STD_VECTOR<TEnum> GetFlagValues() { 

            PP_MACRO_STD_VECTOR<TEnum> res;

            for(int i = 0; i < 32; i++)
            {
                if(((1 << i) & m_AllFlags()) != 0)
                    res.push_back((TEnum)(1 << i));
            }

            return res;
        }

        static TEnum IntegerToFlags(int inProbableEnum) 
        { 
            TEnum res;

            if(!TryIntegerToFlags(inProbableEnum, &res))
                PP_MACRO_ENUM_ARG_EXCEPTION_RAISE("Given flags value is not valid!");

            return res;
        }
    };
PP_MACRO_METAENUM_NAMESPACE_DEFEND

PP_MACRO_NAMESPACE_BEGIN(PP_MACRO_METAENUM_ENUM_NAMESPACE)

    // is the given integer value a valid enumeration member?
    template<class TEnum>
    bool isValid(int inProbableEnum) { return PP_MACRO_METAENUM_NAMESPACE_NAME::EnumSupport<TEnum>::IsValidEnum(inProbableEnum); }

    // returns a string list with all member values for a specific enumeration
    template<class TEnum>
    PP_MACRO_STD_VECTOR<TEnum> getValues() { return PP_MACRO_METAENUM_NAMESPACE_NAME::EnumSupport<TEnum>::GetEnumValues(); }

    // returns a string list with all member identifiers for a specific enumeration
    template<class TEnum>
    PP_MACRO_STD_VECTOR<PP_MACRO_STD_STRING> getNames() { return PP_MACRO_METAENUM_NAMESPACE_NAME::EnumSupport<TEnum>::GetEnumNames(); }

    // looks for the enum member identifier that maps to the given value and returns it string representation
    template<class TEnum>
    PP_MACRO_STD_STRING toString(TEnum inEnum) { return PP_MACRO_METAENUM_NAMESPACE_NAME::EnumSupport<TEnum>::EnumToString(inEnum); }

    // Safely converts the given string to an enumeration value by member identifier lookup (case-insensitive).
    // Throws "PP_MACRO_ENUM_ARG_EXCEPTION" if conversion fails.
    template<class TEnum>
    TEnum fromString(PP_MACRO_STD_STRING inString) { return PP_MACRO_METAENUM_NAMESPACE_NAME::EnumSupport<TEnum>::StringToEnum(inString); }

    template<class TEnum>
    bool tryParse(PP_MACRO_STD_STRING inProbableEnum, TEnum* outValue) { return PP_MACRO_METAENUM_NAMESPACE_NAME::EnumSupport<TEnum>::TryStringToEnum(inProbableEnum, outValue); }

    template<class TEnum>
    bool tryParse(int inProbableEnum, TEnum* outValue) { return PP_MACRO_METAENUM_NAMESPACE_NAME::EnumSupport<TEnum>::TryIntegerToEnum(inProbableEnum, outValue); }

    // Safely converts the given int to an enumeration value by member value lookup.
    // Throws "PP_MACRO_ENUM_ARG_EXCEPTION" if conversion fails.
    template<class TEnum>
    TEnum fromInt(int inProbableEnum) { return PP_MACRO_METAENUM_NAMESPACE_NAME::EnumSupport<TEnum>::IntegerToEnum(inProbableEnum); }

PP_MACRO_NAMESPACE_END(PP_MACRO_METAENUM_ENUM_NAMESPACE)


PP_MACRO_NAMESPACE_BEGIN(PP_MACRO_METAENUM_FLAGS_NAMESPACE)

    // is the given integer value a valid flags value?
    template<class TEnum>
    bool areValid(int inProbableEnum) { return PP_MACRO_METAENUM_NAMESPACE_NAME::EnumSupport<TEnum>::AreValidFlags(inProbableEnum); }

    // is the given flag "inFlagToCheck" set in "inFlags"?
    template<class TEnum>
    bool isSet(TEnum inFlags, TEnum inFlagToCheck) { return PP_MACRO_METAENUM_NAMESPACE_NAME::EnumSupport<TEnum>::IsFlagSet(inFlags, inFlagToCheck); }

    // is any of the given flags "inFlagToCheck" set in "inFlags"?
    template<class TEnum>
    bool isAnySet(TEnum inFlags, TEnum inFlagsToCheck) { return PP_MACRO_METAENUM_NAMESPACE_NAME::EnumSupport<TEnum>::IsAnyFlagSet(inFlags, inFlagsToCheck); }

    // are all the given flags "inFlagToCheck" set in "inFlags"?
    template<class TEnum>
    bool areAllSet(TEnum inFlags, TEnum inFlagsToCheck) { return PP_MACRO_METAENUM_NAMESPACE_NAME::EnumSupport<TEnum>::AreAllFlagsSet(inFlags, inFlagsToCheck); }

    // returns a list of enumeration members that are set in "inFlags"? Useful for ToString() implementations...
    template<class TEnum>
    PP_MACRO_STD_VECTOR<TEnum> decompose(TEnum inFlags) { return PP_MACRO_METAENUM_NAMESPACE_NAME::EnumSupport<TEnum>::DecomposeFlags(inFlags); }

    // returns a list of all valid flag values. Useful for FromString() implementations...
    template<class TEnum>
    PP_MACRO_STD_VECTOR<TEnum> getValues() { return PP_MACRO_METAENUM_NAMESPACE_NAME::EnumSupport<TEnum>::GetFlagValues(); }

    template<class TEnum>
    bool tryParse(int inProbableEnum, TEnum* outValue) { return PP_MACRO_METAENUM_NAMESPACE_NAME::EnumSupport<TEnum>::TryIntegerToFlags(inProbableEnum, outValue); }

    // Safely converts the given int to a flags value by member value lookup.
    // Throws "PP_MACRO_ENUM_ARG_EXCEPTION" if conversion fails.
    template<class TEnum>
    TEnum fromInt(int inProbableEnum) { return PP_MACRO_METAENUM_NAMESPACE_NAME::EnumSupport<TEnum>::IntegerToFlags(inProbableEnum); }

    // TODO: replace with variadic template if available
    #define __PP_MACRO_FUNC_ARG_CAT(z, n, text) BOOST_PP_CAT(text, n),
    #define __PP_MACRO_FUNC_CONV_CAT(z, n, text) BOOST_PP_CAT(text, n) |
    #define __PP_MACRO_FUNC_REP(z, n, text) __PP_MACRO_FUNC_Of(n)

    #define __PP_MACRO_FUNC_Of(N)\
        template<class TEnum> TEnum Of(\
            BOOST_PP_REPEAT_FROM_TO(1, N, __PP_MACRO_FUNC_ARG_CAT, TEnum f) TEnum BOOST_PP_CAT(f, N)) { \
            return (TEnum)( BOOST_PP_REPEAT_FROM_TO(1, N, __PP_MACRO_FUNC_CONV_CAT, (int)f) (int)BOOST_PP_CAT(f, N) );\
        }

    BOOST_PP_REPEAT_FROM_TO(1, 10, __PP_MACRO_FUNC_REP, _)

    #undef __PP_MACRO_FUNC_CONV_CAT
    #undef __PP_MACRO_FUNC_REP
    #undef __PP_MACRO_FUNC_Of
    #undef __PP_MACRO_FUNC_ARG_CAT

PP_MACRO_NAMESPACE_END(PP_MACRO_METAENUM_FLAGS_NAMESPACE)



/*
    The following defined the underlying data type. You may want to change the semantics
    of the workaround structure, in case of incompatibilities.
*/
#if !CFLAGS_ENABLE_ENUM_CLASS_WORKAROUND
    #define PP_MACRO_ENUM_DECLARATION(Namespace, TEnum, DefaultValue, ...)            \
        PP_MACRO_NAMESPACE_BEGIN(Namespace)                            \
            enum class TEnum                                        \
            {                                                        \
                __VA_ARGS__                                            \
            };                                                        \
        PP_MACRO_NAMESPACE_END(Namespace) 

#else
    #define PP_MACRO_ENUM_DECLARATION(Namespace, TEnum, DefaultValue, ...)                \
        PP_MACRO_NAMESPACE_BEGIN(Namespace)                                                \
            struct TEnum                                                                \
            {                                                                            \
                enum TValue {                                                            \
                    __VA_ARGS__                                                            \
                };                                                                        \
            private:                                                                    \
                int value;                                                                \
            public:                                                                        \
                TEnum() { value = DefaultValue; }                                        \
                TEnum(int in) { value = in; }                                            \
                TEnum(TValue in) { value = in; }                                        \
                operator int() const { return value; }                                    \
                operator size_t() const { return value; }                                \
                bool operator ==(TEnum other) const { return value == other.value; }    \
                bool operator ==(TValue other) const { return value == other; }            \
                bool operator ==(int other) const { return (int)value == other; }        \
                bool operator !=(TEnum other) const { return value != other.value; }    \
                bool operator !=(TValue other) const { return value != other; }            \
                bool operator !=(int other) const { return (int)value != other; }        \
            };                                                                            \
        PP_MACRO_NAMESPACE_END(Namespace) 
#endif
