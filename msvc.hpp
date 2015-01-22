//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  msvc.hpp
//
//   Description:  MSVC 一些基本配制
//
//       Version:  1.0
//       Created:  2014年11月27日 09时50分30秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once

#include<string>

#ifdef _MSC_VER

#define constexpr
#define alignas(Int)

#ifdef noexcept
    #undef noexcept
#endif
#define noexcept(Val)

namespace core
{
    std::wstring mbrtowc(const std::string& src)
    {
        std::wstring ret(src.size(), L'\0');
        const char* ptr=src.c_str();
        std::mbstate_t state = std::mbstate_t();
        const auto len = std::mbsrtowcs(wc.data(), &ptr, ret.size(), &state);
        if(static_cast<std::size_t>(-1)!=len)
            ret.resize(len);
        return ret;
    }
}

#endif  // _MSC_VER

