//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  value.hpp
//
//   Description:  统一值类型
//
//       Version:  1.0
//       Created:  2015年05月14日 11时36分45秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once

#include<cstring>
#include<cstdint>
#include<cassert>

#include<vector>
#include<string>

namespace core
{

enum class SimpleValue_t: uint8_t
{
    eNone= 0,
    eInt,
    eInt64,
    eIntList,
    eInt64List,
    eString,
    eStringList,
};

namespace details
{

template<SimpleValue_t eType>
struct SimpleValueTypeMapT;

template<>
struct SimpleValueTypeMapT<SimpleValue_t::eInt>
{
    typedef int32_t Value;
};

template<>
struct SimpleValueTypeMapT<SimpleValue_t::eInt64>
{
    typedef int64_t Value;
};

template<>
struct SimpleValueTypeMapT<SimpleValue_t::eIntList>
{
    typedef std::vector<int32_t> Value;
};

template<>
struct SimpleValueTypeMapT<SimpleValue_t::eInt64List>
{
    typedef std::vector<int64_t> Value;
};

template<>
struct SimpleValueTypeMapT<SimpleValue_t::eString>
{
    typedef std::string Value;
};

template<>
struct SimpleValueTypeMapT<SimpleValue_t::eStringList>
{
    typedef std::vector<std::string> Value;
};

}

class SimpleValue
{
public:
    typedef std::vector<int32_t> IntList;
    typedef std::vector<int64_t> Int64List;
    typedef std::vector<std::string> StringList;

private:
    enum
    {
#define MMax(x, y) (x>y?x:y)
        eSizeString=sizeof(std::string),
        eSizeStringList=sizeof(StringList),
        eSizeInt=sizeof(int32_t),
        eSizeIntList=sizeof(IntList),

        eUnionSize= MMax(eSizeString, MMax(eSizeStringList, MMax(eSizeInt, eSizeIntList)))
#undef MMax
    };

    struct Value
    {
        char data[eUnionSize];
    };

public:
    static const SimpleValue& null()
    {
        static SimpleValue gs;
        return gs;
    }

    SimpleValue_t tag() const
    {
        return static_cast<SimpleValue_t>(tag_);
    }

    template<typename... Args>
    explicit SimpleValue(Args&&... args)
    {
        init(std::forward<Args&&>(args)...);
    }

    void init()
    {
        std::memset(this, 0, sizeof(*this));
    }

    void init(const SimpleValue& other);
    void init(SimpleValue&& other);

    template<typename Int>
    void init(Int val,
        typename std::enable_if<std::is_integral<Int>::value && (sizeof(Int)<=4)>::type* = nullptr
    )
    {
        init(static_cast<int32_t>(val));
    }

    template<typename Int>
    void init(Int val,
        typename std::enable_if<std::is_integral<Int>::value && (sizeof(Int)>4)>::type* = nullptr
    )
    {
        init(static_cast<int64_t>(val));
    }

    void init(int32_t val)
    {
        tag_=SimpleValue_t::eInt;
        new (pas<int32_t>()) int32_t(val);
    }

    void init(int64_t val)
    {
        tag_=SimpleValue_t::eInt64;
        new (pas<int64_t>()) int64_t(val);
    }

    void init(const char* s, size_t n)
    {
        tag_=SimpleValue_t::eString;
        new (pas<std::string>()) std::string(s, n);
    }

    void init(const char* s)
    {
        init(s, std::strlen(s));
    }

    void init(const std::string& s)
    {
        tag_=SimpleValue_t::eString;
        new (pas<std::string>()) std::string(s.c_str(), s.size());
    }

    void init(std::string&& s)
    {
        tag_=SimpleValue_t::eString;
        new (pas<std::string>()) std::string(std::forward<std::string&&>(s));
    }

    template<typename Itr>
    void init(Itr itr, Itr end,
        typename std::enable_if<
            std::is_integral<typename std::iterator_traits<Itr>::value_type>::value
            && (sizeof(typename std::iterator_traits<Itr>::value_type)>4)
        >::type* =nullptr
    )
    {
        tag_=SimpleValue_t::eInt64List;
        new (pas<Int64List>()) Int64List(itr, end);
    }

template<typename Itr>
    void init(Itr itr, Itr end,
        typename std::enable_if<
            std::is_integral<typename std::iterator_traits<Itr>::value_type>::value
            && sizeof(typename std::iterator_traits<Itr>::value_type)<=4
        >::type* =nullptr
    )
    {
        tag_=SimpleValue_t::eIntList;
        new (pas<IntList>()) IntList(itr, end);
    }

    template<typename Itr>
    void init(Itr itr, Itr end,
        typename std::enable_if<std::is_constructible<StringList, Itr, Itr>::value
            && !std::is_integral<typename std::iterator_traits<Itr>::value_type>::value>::type* =nullptr)
    {
        tag_=SimpleValue_t::eStringList;
        new (pas<StringList>()) StringList(itr, end);
    }

    void init(const IntList& v)
    {
        tag_=SimpleValue_t::eIntList;
        new (pas<IntList>()) IntList(v);
    }

    void init(IntList&& v)
    {
        tag_=SimpleValue_t::eIntList;
        new (pas<IntList>()) IntList(std::forward<IntList&&>(v));
    }

    void init(const Int64List& v)
    {
        tag_=SimpleValue_t::eInt64List;
        new (pas<Int64List>()) Int64List(v);
    }

    void init(Int64List&& v)
    {
        tag_=SimpleValue_t::eInt64List;
        new (pas<Int64List>()) Int64List(std::forward<Int64List&&>(v));
    }

    void init(const StringList& v)
    {
        tag_=SimpleValue_t::eStringList;
        new (pas<StringList>()) StringList(v);
    }

    void init(StringList&& v)
    {
        tag_=SimpleValue_t::eStringList;
        new (pas<StringList>()) StringList(std::forward<StringList&&>(v));
    }

    template<typename V>
    SimpleValue& operator=(V&& o)
    {
        reset();
        init(std::forward<V&&>(o));
        return *this;
    }

    SimpleValue& operator=(const SimpleValue& o)
    {
        reset();
        init(o);
        return *this;
    }

    SimpleValue& operator=(SimpleValue& o)
    {
        reset();
        init(std::move(o));
        return *this;
    }

    bool operator==(const SimpleValue& o) const;

    template<typename... Args>
    void assign(Args&&... args)
    {
        reset();
        init(std::forward<Args&&>(args)...);
    }

    template<SimpleValue_t eType>
    const typename details::SimpleValueTypeMapT<eType>::Value& get() const
    {
        assert(tag_==eType);
        return as<typename details::SimpleValueTypeMapT<eType>::Value>();
    }

    int32_t intGet() const
    {
        return get<SimpleValue_t::eInt>();
    }

    const std::string& strGet() const
    {
        return get<SimpleValue_t::eString>();
    }

    void reset();

    void encode(std::string& dest) const;
    void decode(uint32_t& start, const std::string& src);

    bool invalid() const
    {
        return tag_==SimpleValue_t::eNone;
    }

    bool valid() const
    {
        return !invalid();
    }

protected:
    template<typename V>
    V& as()
    {
        return *pas<V>();
    }

    template<typename V>
    const V& as() const
    {
        return *pas<V>();
    }

    template<typename V>
    V* pas()
    {
        return reinterpret_cast<V*>(value_.data);
    }

    template<typename V>
    const V* pas() const
    {
        return reinterpret_cast<const V*>(value_.data);
    }

    template<typename T>
    void destructe(T& t)
    {
        t.~T();
    }

protected:
    SimpleValue_t tag_;
    Value value_;
};

}

