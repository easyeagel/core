//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  encode.hpp
//
//   Description:  编码解决
//
//       Version:  1.0
//       Created:  2014年12月31日 11时09分44秒
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

#include"utf8/utf8.h"

namespace core
{
namespace details
{
    template<size_t WCharN>
    class WCharConverter;

    template<>
    class WCharConverter<2>
    {
    public:
        static std::wstring  from(const char* buf, size_t n)
        {
            std::wstring ret;
            utf8::utf8to16(buf, buf+n, std::back_inserter(ret));
            return std::move(ret);
        }

        static std::wstring from(const std::string& str)
        {
            return from(str.data(), str.size());
        }

        static std::wstring from(const std::wstring& str)
        {
            return str;
        }

        static wchar_t from(char c)
        {
            wchar_t ret=0;
            utf8::utf8to16(&c, &c+1, &ret);
            return ret;
        }

        static std::string to(const wchar_t* buf, size_t n)
        {
            std::string ret;
            utf8::utf16to8(buf, buf+n, std::back_inserter(ret));
            return std::move(ret);
        }

        static std::string to(const std::wstring& src)
        {
            return to(src.data(), src.size());
        }

        static std::string to(const std::string& src)
        {
            return src;
        }
    };

    template<>
    class WCharConverter<4>
    {
    public:
        static std::wstring  from(const char*    buf, size_t n)
        {
            std::wstring ret;
            utf8::utf8to32(buf, buf+n, std::back_inserter(ret));
            return std::move(ret);
        }

        static std::wstring from(const std::string& str)
        {
            return from(str.data(), str.size());
        }

        static std::wstring from(const std::wstring& str)
        {
            return str;
        }

        static wchar_t from(char c)
        {
            wchar_t ret=0;
            utf8::utf8to32(&c, &c+1, &ret);
            return ret;
        }

        static std::string to(const wchar_t* buf, size_t n)
        {
            std::string ret;
            utf8::utf32to8(buf, buf+n, std::back_inserter(ret));
            return std::move(ret);
        }

        static std::string to(const std::wstring& src)
        {
            return to(src.data(), src.size());
        }

        static std::string to(const std::string& src)
        {
            return src;
        }

    };

}

class WCharConverter: public details::WCharConverter<sizeof(wchar_t)>
{};

void utf8Enable(std::wios& io);

}

