//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  int.hpp
//
//   Description:  整数相关操作
//
//       Version:  1.0
//       Created:  2013年12月13日 11时44分18秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================

#pragma once

#include<cstdio>
#include<cassert>
#include<cstdint>

#include<string>
#include<ostream>

#include"msvc.hpp"
#include"operator.hpp"

namespace core
{

struct IntHash;

template<typename Object, typename Int>
class alignas(Int) IntValueT: public TotalOrderT<Object>
{
    static const Int eInvalid=~static_cast<Int>(0);
public:
    typedef Int Value_t;

    typedef IntHash Hash;

    IntValueT()
        :value_(eInvalid)
    {
        static_assert(sizeof(Object)==sizeof(Int), "alignas error");
    }

    IntValueT(Value_t v)
        :value_(v)
    {
    }

    static bool valid(Value_t id)
    {
        return id!=eInvalid;
    }

    bool valid() const
    {
        return value_!=eInvalid;
    }

    bool invalid() const
    {
        return value_==eInvalid;
    }

    bool empty() const
    {
        return value_==eInvalid;
    }

    bool operator<(const Object& mid) const
    {
        assert(!empty() && !mid.empty());
        return value_<mid.value_;
    }

    bool operator==(const Object& mid) const
    {
        return value_==mid.value_;
    }

    Value_t valueGet() const
    {
        assert(!empty());
        return value_;
    }

    Value_t valueGetIf(Value_t val=eInvalid) const
    {
        if(empty())
            return val;
        return value_;
    }

    void clear()
    {
        value_=eInvalid;
    }

    std::string toString(size_t width=0, const char* prefix="", const char* subfix="") const
    {
        auto str=std::to_string(valueGet());
        if(str.size()<width)
            str.insert(str.begin(), width-str.size(), '0');
        return prefix+str+subfix;
    }

    explicit operator Value_t()
    {
        return value_;
    }

    Value_t encode() const
    {
        return value_;
    }

    void decode(Value_t val)
    {
        value_=val;
    }

    typedef Value_t Compact;

    static constexpr Value_t invalidGet()
    {
        return eInvalid;
    }

    Value_t& refGet()
    {
        return value_;
    }

    const Value_t& refGet() const
    {
        return value_;
    }
private:
    Value_t value_;
};

struct IntHash
{
    template<typename Object, typename Int>
    std::size_t operator()(const mr::IntValueT<Object, Int>& val) const
    {
        return val.valueGetIf();
    }
};

template<typename Object, typename Enum>
class alignas(Enum) EnumValueT: public TotalOrderT<Object>
{
public:
    typedef Enum Value_t;

    EnumValueT()
        :value_(Enum::eEnumNone)
    {
        static_assert(Enum::eEnumNone<Enum::eEnumCount , "value error");
        static_assert(sizeof(Object)==sizeof(Enum), "alignas error");
    }

    EnumValueT(Value_t v)
        :value_(v)
    {}

    template<typename Int>
    static bool valid(Int val)
    {
        return static_cast<Value_t>(val)>=Enum::eEnumNone && static_cast<Value_t>(val)<Enum::eEnumCount;
    }

    bool valid() const
    {
        return valid(value_);
    }

    bool invalid() const
    {
        return !valid();
    }

    bool empty() const
    {
        return invalid();
    }

    bool operator<(const Object& mid) const
    {
        assert(!empty() && !mid.empty());
        return value_<mid.value_;
    }

    bool operator==(const Object& mid) const
    {
        return value_==mid.value_;
    }

    Value_t valueGet() const
    {
        assert(valid());
        return value_;
    }

    Value_t valueGetIf(Value_t val=Enum::eEnumNone) const
    {
        if(empty())
            return val;
        return value_;
    }

    void clear()
    {
        value_=Enum::eEnumNone;
    }

    explicit operator Value_t()
    {
        return value_;
    }

    static constexpr size_t countGet()
    {
        return static_cast<size_t>(Enum::eEnumCount);
    }

    Value_t& refGet()
    {
        return value_;
    }

    const Value_t& refGet() const
    {
        return value_;
    }
private:
    Value_t value_;
};



template<typename Int>
Int circleInt(Int total, Int start, Int offset)
{
    return (start+offset)%total;
}

template<typename Object, typename Int>
std::ostream& operator<<(std::ostream& stm, const IntValueT<Object, Int>& obj)
{
    stm << obj.valueGet();
    return stm;
}

template<typename Object>
std::ostream& operator<<(std::ostream& stm, const IntValueT<Object, uint8_t>& obj)
{
    stm << static_cast<unsigned>(obj.valueGet());
    return stm;
}

template<typename Object>
std::ostream& operator<<(std::ostream& stm, const IntValueT<Object, int8_t>& obj)
{
    stm << static_cast<int>(obj.valueGet());
    return stm;
}

class IntCode
{
public:
    template<typename... Args>
    static std::string& encode(std::string& dest, Args... args)
    {
        return encodeImpl(dest, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static char* encode(char* dest, Args... args)
    {
        return encodeImpl(dest, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static size_t decode(const std::string& dest, Args&... args)
    {
        return decodeImpl(static_cast<size_t>(0), dest, std::forward<Args&>(args)...);
    }

    template<typename... Args>
    static size_t decode(size_t start, const std::string& dest, Args&... args)
    {
        return decodeImpl(start, dest, std::forward<Args&>(args)...);
    }

    template<typename Return>
    static Return decode(const std::string& dest)
    {
        Return ret=0;
        decode(dest, ret);
        return ret;
    }

    template<typename Return>
    static Return decode(size_t start, const std::string& dest)
    {
        Return ret=0;
        decode(start, dest, ret);
        return ret;
    }

    template<typename... Args>
    static const char* decode(const char* dest, Args&... args)
    {
        return decodeImpl(dest, std::forward<Args&>(args)...);
    }

    template<typename Return>
    static Return decode(const char* dest)
    {
        Return ret=0;
        decode(dest, ret);
        return ret;
    }

private:
    template<typename Int, typename... Args>
    static std::string& encodeImpl(std::string& dest, Int val, Args... args)
    {
        encodeImpl(dest, val);
        encodeImpl(dest, std::forward<Args>(args)...);
        return dest;
    }

    template<typename Int>
    static std::string& encodeImpl(std::string& dest, Int val)
    {
        auto const sz=dest.size();
        dest.resize(sz+sizeof(val));
        encodeImpl(&dest[sz], val);
        return dest;
    }

    template<typename Int, typename... Args>
    static char* encodeImpl(char* dest, Int val, Args... args)
    {
        encodeImpl(dest, val);
        return encodeImpl(dest+sizeof(val), std::forward<Args>(args)...);
    }

    template<typename Int>
    static char* encodeImpl(char* dest, Int val)
    {
        typedef typename std::make_unsigned<Int>::type Rlt;
        for(size_t i=0; i<sizeof(Int); ++i)
            dest[i]=static_cast<char>(static_cast<Rlt>(val) >> ((sizeof(Int)-i-1)*8));
        return dest+sizeof(val);
    }

    template<typename Int, typename... Args>
    static size_t decodeImpl(size_t start, const std::string& dest, Int& val, Args&... args)
    {
        decodeImpl(start, dest, val);
        return sizeof(Int)+decodeImpl(start+sizeof(Int), dest, std::forward<Args&>(args)...);
    }

    template<typename Int>
    static size_t decodeImpl(size_t start, const std::string& dest, Int& val)
    {
        decodeImpl(&dest[0]+start, val);
        return sizeof(val);
    }

    template<typename Int, typename... Args>
    static const char* decodeImpl(const char* dest, Int& val, Args&... args)
    {
        decodeImpl(dest, val);
        return decodeImpl(dest+sizeof(val), std::forward<Args&>(args)...);
    }

    template<typename Int>
    static const char* decodeImpl(const char* dest, Int& val)
    {
        typedef typename std::make_unsigned<Int>::type Rlt;

        Rlt rlt=0;
        for(size_t i=0; i<sizeof(Int); ++i)
            rlt |= static_cast<Rlt>(static_cast<uint8_t>(dest[i])) << ((sizeof(Int)-i-1)*8);
        val=static_cast<Int>(rlt);
        return dest+sizeof(Int);
    }

};

}


