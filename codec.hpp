//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  codec.hpp
//
//   Description:  存储序列化
//
//       Version:  1.0
//       Created:  2015年05月15日 11时32分16秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once

#include"int.hpp"

namespace core
{

struct CodecImpl
{
    template<typename Int>
    static void encode(std::string& dest, Int val,
        typename std::enable_if<std::is_integral<Int>::value>::type* =nullptr)
    {
        IntCode::encode(dest, val);
    }

    template<typename Int>
    static void encode(std::string& dest, Int val,
        typename std::enable_if<std::is_enum<Int>::value>::type* =nullptr)
    {
        IntCode::encode(dest, static_cast<typename std::underlying_type<Int>::type>(val));
    }

    static void encode(std::string& dest, const std::string& src)
    {
        IntCode::encode(dest, static_cast<uint32_t>(src.size()));
        dest += src;
    }

    static void encode(std::string& dest, bool v)
    {
        IntCode::encode(dest, static_cast<uint8_t>(v?1:0));
    }

    static void encode(std::string& dest, const std::wstring& src)
    {
        IntCode::encode(dest, static_cast<uint32_t>(src.size()));
        for(auto c: src)
            IntCode::encode(dest, static_cast<uint32_t>(c));
    }

    template<typename Value>
    static void encode(std::string& dest, const Value& val,
        typename std::enable_if<!std::is_enum<Value>::value && !std::is_integral<Value>::value>::type* =nullptr)
    {
        val.encode(dest);
    }

    template<typename Int>
    static void decode(uint32_t& start, const std::string& src, Int& val,
        typename std::enable_if<std::is_integral<Int>::value>::type* =nullptr)
    {
        IntCode::decode(start, src, val);
        start += sizeof(val);
    }

    template<typename Int>
    static void decode(uint32_t& start, const std::string& src, Int& val,
        typename std::enable_if<std::is_enum<Int>::value>::type* =nullptr)
    {
        typename std::underlying_type<Int>::type v;
        IntCode::decode(start, src, v);
        start += sizeof(v);
        val=static_cast<Int>(v);
    }

    static void decode(uint32_t& start, const std::string& src, std::string& dest)
    {
        uint32_t len=0;
        IntCode::decode(start, src, len);
        start += sizeof(len);

        dest.assign(src.substr(start, len));
        start += len;
    }

    static void decode(uint32_t& start, const std::string& src, bool& dest)
    {
        uint8_t v=0;
        IntCode::decode(start, src, v);
        start += sizeof(v);

        dest=(v==0?false:true);
    }

    static void decode(uint32_t& start, const std::string& src, std::wstring& dest)
    {
        uint32_t len=0;
        IntCode::decode(start, src, len);
        start += sizeof(len);

        for(uint32_t i=0; i<len; ++i)
        {
            uint32_t c=0;
            IntCode::decode(start, src, c);
            start += sizeof(c);

            dest.push_back(static_cast<wchar_t>(c));
        }
    }

    template<typename Value>
    static void decode(uint32_t& start, const std::string& src, Value& val,
        typename std::enable_if<!std::is_enum<Value>::value && !std::is_integral<Value>::value>::type* =nullptr)
    {
        val.decode(start, src);
    }

};

static inline void encode(std::string& )
{}

template<typename First, typename... Args>
inline void encode(std::string& dest, First&& first, Args&&... args)
{
    CodecImpl::encode(dest, std::forward<First&&>(first));
    encode(dest, std::forward<Args&&>(args)...);
}

template<typename First, typename... Args>
inline std::string encode(First&& first, Args&&... args)
{
    std::string ret;
    encode(ret, std::forward<First&&>(first), std::forward<Args&&>(args)...);
    return std::move(ret);
}


static inline void decode(uint32_t& , const std::string& )
{}

template<typename First, typename... Args>
inline void decode(uint32_t& start, const std::string& src, First&& first, Args&&... args)
{
    CodecImpl::decode(start, src, std::forward<First&&>(first));
    decode(start, src, std::forward<Args&&>(args)...);
}

template<typename... Args>
inline void decode(const std::string& src, Args&&... args)
{
    uint32_t start=0;
    decode(start, src, std::forward<Args&&>(args)...);
}

template<typename Value>
inline Value decode(uint32_t& start, const std::string& src)
{
    Value ret;
    decode(start, src, ret);
    return std::move(ret);
}

template<typename Value>
inline Value decode(const std::string& src)
{
    Value ret;
    uint32_t start=0;
    decode(start, src, ret);
    return std::move(ret);
}

struct Compact
{

template<typename T, typename Compact=typename T::Compact>
static inline T decode(const Compact& c)
{
    T t;
    t.decompact(c);
    return std::move(t);
}

template<typename T, typename Compact=typename T::Compact>
static inline Compact encode(const T& t)
{
    return t.compact();
}
};

}

