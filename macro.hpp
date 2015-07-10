//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  macro.hpp
//
//   Description:  easyDB 通用宏定义
//
//       Version:  1.0
//       Created:  2013年03月12日 13时38分25秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================

#pragma once

#include<cstdio>
#include<cstdlib>
#include<cstring>

#include<string>
#include<boost/detail/endian.hpp>


//============================================================
#define GMacroStrCat_(CppStart, CppEnd) CppStart ## CppEnd
#define GMacroStrCat(CppStart, CppEnd) GMacroStrCat_(CppStart, CppEnd)

//============================================================
#define GMacroStr_(CppValue) #CppValue
#define GMacroStr(CppValue) GMacroStr_(CppValue)

//============================================================

static inline std::string stringMake(const char* s1, const char* s2)
{
    return std::string(s1) + s2;
}

static inline std::string stringMake(const char* s1, const char* s2, const char* s3)
{
    return stringMake(s1, s2) + s3;
}

static inline std::string stringMake(std::string&& s1, const char* s2, const char* s3)
{
    return s1+s2+s3;
}

static inline std::string stringExclude(const char* s1, const char* exclude)
{
    const auto sz=std::strlen(exclude);
    if(std::strncmp(s1, exclude, sz)==0)
        return std::string(s1+sz);
    return std::string(s1);
}

#ifndef GMacroSourceRoot
    #define GMacroWhere stringMake(__FILE__, ":", GMacroStr(__LINE__))
#else //GMacroSourceRoot
    #define GMacroWhere stringMake(stringExclude(__FILE__, GMacroStr(GMacroSourceRoot)), ":", GMacroStr(__LINE__))
#endif //GMacroSourceRoot
#define GMacroWhereMsg(CppMsg) stringMake(GMacroWhere, ":", CppMsg)


//============================================================
//一个不使用变量
#define GMacroUnUsedVar(CppVar) (void)CppVar;


//============================================================
//声明一个类的构造函数的默认值，复制运算符的默认值
#define GMacroClassDefault(CppClass) \
    CppClass()=default; \
    CppClass(const CppClass& )=default; \
    CppClass(CppClass&& )=default; \
    CppClass& operator=(const CppClass& )=default; \
    CppClass& operator=(CppClass&& )=default;


#ifdef _MSC_VER
#define GMacroBaseThis(CppClass) template<typename... Args> CppClass(Args&&... args): BaseThis(std::forward<Args&&>(args)...) {}
#else
#define GMacroBaseThis(CppClass) using BaseThis::BaseThis;
#endif


//============================================================
static inline void printAbort(const char* file, int line, const char* msg)
{
    std::fprintf(stderr, "%s:%d:%s", file, line, msg);
    std::fflush(stderr);
    std::abort();
}

#define GMacroAbort(CppMsg) printAbort(__FILE__, __LINE__, CppMsg);


//============================================================
#ifndef GMacroFunName
    #if defined(EasyGCC)
        #define GMacroFunName __PRETTY_FUNCTION__
    #elif defined(EasyMSVC)
        #define GMacroFunName __FUNCSIG__
    #else
        #define GMacroFunName __func__
    #endif
#endif



#ifndef GMacroFunMsg
    #if defined(GMacroNoFunMsg)
        #define GMacroFunMsg do{}while(mr::compileConst(0));
    #else
        #define GMacroFunMsg do{ \
                std::fprintf(stderr, "%s", (std::string(GMacroFunName) + "\n").c_str()); \
            }while(mr::compileConst(0));
    #endif
#endif


//=================================================================
//一个简单在线的endian选择器
#ifdef BOOST_LITTLE_ENDIAN
    #define GMacroBigEndianIf(CppBig, CppLittle) CppLittle
#else
    #define GMacroBigEndianIf(CppBig, CppLittle) CppBig
#endif


//=================================================================
//当是调试模式时，调用执行某代码
#ifdef DEBUG
    #define GMacroDebugDoit(CppFun) CppFun()
#else
    #define GMacroDebugDoit(CppFun) (void)
#endif

