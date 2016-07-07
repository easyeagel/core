//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  string.hpp
//
//   Description:  字符串相关处理
//
//       Version:  1.0
//       Created:  2013年06月07日 13时54分54秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:
//
//=====================================================================================



#pragma once

#include<cstring>
#include<cassert>

#include<iosfwd>
#include<string>
#include<utility>
#include<algorithm>

#include"msvc.hpp"
#include"macro.hpp"
#include"codec.hpp"
#include"typedef.hpp"
#include"operator.hpp"

#ifdef min
    #undef min
#endif

namespace core
{

#define GMacroConstStr(CppType, CppStr) \
    struct CppType \
    { \
        static constexpr const char* value=CppStr; \
    }


struct ConstString
{
    static unsigned constexpr hash(const char* v)
    {
        return hash(v, v+length(v));
    }

    static unsigned constexpr length(const char* v)
    {
        return length(v, 0);
    }

private:
    static unsigned constexpr length(const char* str, unsigned sum)
    {
        return *str==0 ? sum : length(str+1, sum+1);
    }

    static unsigned constexpr hash(char const* const input, char const* const end, unsigned sum=5381u)
    {
        return (input==end)
            ? sum
            : hash(input, end-1, static_cast<unsigned>(*(end-1))+33u*sum);
    }

};

static inline unsigned constexpr constHash(char const *input)
{
    return ConstString::hash(input);
}

static inline unsigned constexpr constLength(char const *input)
{
    return ConstString::length(input);
}

template<typename Itr>
inline unsigned stringHash(Itr input, Itr end)
{
    unsigned sum=5381u;
    for(--end; end>=input; --end)
        sum = static_cast<unsigned>(*end) + 33u*sum;
    return sum;
}


//============================================================================================
//实用工具
class StringHex
{
public:
    static constexpr size_t encodeSizeGet(size_t sz)
    {
        return sz*2;
    }

    static constexpr size_t decodeSizeGet(size_t sz)
    {
        return sz/2;
    }

    static void encode(char const * input, char const* const end, char* dest);
    static void decode(char const * input, char const* const end, char* dest);

    static std::string encode(const std::string& input)
    {
        std::string dest(encodeSizeGet(input.size()), '\0');
        encode(input.c_str(), input.c_str()+input.size(), const_cast<char*>(dest.data()));
        return std::move(dest);
    }

    static std::string decode(const std::string& input)
    {
        std::string dest(decodeSizeGet(input.size()), '\0');
        decode(input.c_str(), input.c_str()+input.size(), const_cast<char*>(dest.data()));
        return std::move(dest);
    }
};



//=======================================================
//指定大小的字符串类型
template<ObjectCount_t SizeMax, typename Count_t=ObjectCount_t>
class FixedStringT
    : public TotalOrderT<FixedStringT<SizeMax, Count_t>>
{
    enum {eMaxSize=SizeMax};
    static_assert((SizeMax+1+sizeof(Count_t))%4==0, "FixedStringT: (SizeMax+1+sizeof(Count_t)) % 4 == 0");
public:
    FixedStringT()
        :size_(0)
    {
        static_assert(sizeof(FixedStringT)==sizeof(Count_t)+eMaxSize+1,  "FixedStringT: sizeof error");
        std::memset(bt_, 0, sizeof(bt_));
    }

    FixedStringT(const char* s, size_t nb)
    {
        assert(eMaxSize>=nb);
        char* const dest=bt_;
        const auto len=std::min(static_cast<size_t>(eMaxSize), nb);
        std::memcpy(dest, s, len);
        std::memset(bt_+len, 0u, sizeof(bt_)-len);
        size_=static_cast<Count_t>(len);
    }

    FixedStringT(const char* s)
        :FixedStringT(s, std::strlen(s))
    {
    }

    FixedStringT(const std::string& s)
        :FixedStringT(s.c_str(), s.size())
    {
    }

    size_t sizeGet() const
    {
        return size_;
    }

    const char* cstrGet() const
    {
        return bt_;
    }

    const char* cstrEnd() const
    {
        return cstrGet()+sizeGet();
    }

    static constexpr size_t maxSizeGet()
    {
        return eMaxSize;
    }


    const FixedStringT& baseGet() const
    {
        return *this;
    }

    FixedStringT& baseGet()
    {
        return *this;
    }

    bool empty() const
    {
        return sizeGet()==0;
    }

    bool operator<(const FixedStringT& right) const
    {
        const uint8_t* s=reinterpret_cast<const uint8_t*>(cstrGet());
        const uint8_t* o=reinterpret_cast<const uint8_t*>(right.cstrGet());
        auto const sz=std::min(right.sizeGet(), sizeGet());
        for(size_t i=0; i<sz; ++i)
        {
            if(s[i]==o[i])
                continue;
            return s[i]<o[i];
        }

        return sizeGet()<right.sizeGet();
    }

    bool operator==(const FixedStringT& right) const
    {
        if(sizeGet()!=right.sizeGet())
            return false;
        return std::memcmp(cstrGet(), right.cstrGet(), sizeGet())==0;
    }

    explicit operator const char* () const
    {
        return cstrGet();
    }

    std::string toString() const
    {
        return std::string(cstrGet(), sizeGet());
    }

    void fromString(const std::string& str)
    {
        *this=str;
    }

    const char* begin() const
    {
        return cstrGet();
    }

    const char* end() const
    {
        return cstrGet()+sizeGet();
    }

    void push(char c)
    {
        assert(sizeGet()<eMaxSize);
        if(sizeGet()>=eMaxSize)
            return;
        bt_[sizeGet()]=c;
        ++size_;
    }

    void push(const char* ptr, size_t n)
    {
        assert(sizeGet()+n<eMaxSize);

        const auto sz=sizeGet();
        const auto newSz=sz+n;
        if(newSz>=eMaxSize)
            return;

        for(size_t i=sz; i<newSz; ++i)
            bt_[i]=*ptr++;

        size_ = static_cast<Count_t>(size_ + n);
    }

    FixedStringT& operator=(char c)
    {
        clear();
        size_=1;
        bt_[0]=c;
        return *this;
    }

    FixedStringT& operator=(const FixedStringT& other)=default;

    void clear()
    {
        std::memset(this, 0, sizeof(*this));
    }

    typedef std::string Compact;

    void encode(std::string& dest) const
    {
        const auto t=toString();
        core::encode(dest, t);
    }

    void decode(uint32_t& start, const std::string& code)
    {
        std::string t;
        core::decode(start, code, t);
        fromString(t);
    }

#ifndef _MSC_VER
    unsigned hash() const
    {
        return stringHash(begin(), end());
    }
#endif  //_MSC_VER

protected:
    char* ptrGet()
    {
        return bt_;
    }

    void sizeSet()
    {
        size_=static_cast<uint8_t>(std::strlen(bt_));
        assert(size_<=eMaxSize);
    }

    void sizeSet(size_t sz)
    {
        assert(sz<=eMaxSize);
        size_=static_cast<uint8_t>(sz);
    }
private:
    Count_t size_;
    char bt_[eMaxSize+1];
};

extern template class FixedStringT<7>;
extern template class FixedStringT<15>;
extern template class FixedStringT<31>;
extern template class FixedStringT<63>;
extern template class FixedStringT<127>;

extern template class FixedStringT<6  , uint8_t>;
extern template class FixedStringT<14 , uint8_t>;
extern template class FixedStringT<18 , uint8_t>;
extern template class FixedStringT<30 , uint8_t>;
extern template class FixedStringT<46 , uint8_t>;
extern template class FixedStringT<62 , uint8_t>;
extern template class FixedStringT<126, uint8_t>;

template<ObjectCount_t SizeMax, typename Count_t=ObjectCount_t>
std::ostream& operator<<(std::ostream& out, const FixedStringT<SizeMax, Count_t>& str)
{
    return out << str.cstrGet();
}

template<ObjectCount_t SizeMax, typename Count_t=ObjectCount_t>
std::string toString(const FixedStringT<SizeMax, Count_t>& str)
{
    return str.toString();
}

template <typename String>
struct StringHash
{
    size_t operator()(const String& str) const
    {
        return str.hash();
    }
};



//=======================================================
//放置式字符串类
class PlaceString
{
    PlaceString()=delete;
    PlaceString(const PlaceString&)=delete;
    PlaceString(PlaceString&&)=delete;
    PlaceString& operator=(const PlaceString&)=delete;
    PlaceString& operator=(PlaceString&&)=delete;

public:
    size_t sizeGet() const
    {
        return size_;
    }

    const char* cstrGet() const
    {
        if(size_)
            return reinterpret_cast<const char*>(&size_+1);
        return "";
    }

    bool operator==(const char* other) const
    {
        return std::strncmp(cstrGet(), other, sizeGet())==0;
    }

    bool operator<(const char* other) const
    {
        return std::strncmp(cstrGet(), other, sizeGet())<0;
    }

    bool operator<=(const char* other) const
    {
        return std::strncmp(cstrGet(), other, sizeGet())<=0;
    }

    template<unsigned SMax>
    FixedStringT<SMax>* cast()
    {
        FixedStringT<SMax>* ptr=reinterpret_cast<FixedStringT<SMax>*>(this);
        if(ptr->sizeGet()>SMax)
            return nullptr;
        return ptr;
    }

    explicit operator const char* () const
    {
        return cstrGet();
    }

private:
    ObjectCount_t size_;
};

namespace details
{

class PlaceStringUnitImpl
{
protected:
    enum {eAlignment=8};
    enum {eInvalidIndex=~0x0u};

public:
    void init(size_t capacity)
    {
        capacity_=static_cast<uint32_t>(capacity);
        free();
    }

    const PlaceString& placeStrGet() const
    {
        return *reinterpret_cast<const PlaceString*>(this+1);
    }

    PlaceString& placeStrGet()
    {
        return *reinterpret_cast<PlaceString*>(this+1);
    }

    void check(size_t sz) const
    {
        GMacroUnUsedVar(sz);
        assert(capacity_-sizeof(PlaceStringUnitImpl)-sizeof(PlaceString)>sz);
    }

    void place(ObjectIndex_t idx, const char* str, size_t sz)
    {
        check(sz);

        index_=static_cast<uint32_t>(idx);

        uint32_t* psz=reinterpret_cast<uint32_t*>(this+1);
        *psz=static_cast<uint32_t>(sz);

        char* dest=reinterpret_cast<char*>(psz+1);
        std::memcpy(dest, str, sz+1);
    }

    bool isFreed() const
    {
        return index_==eInvalidIndex;
    }

    ObjectCount_t capacityGet() const
    {
        return capacity_;
    }

    ObjectIndex_t indexGet() const
    {
        return index_;
    }

    void free()
    {
        index_=eInvalidIndex;
    }

private:
    ObjectCount_t capacity_;
    ObjectIndex_t index_; //记住自己的偏移，因为重新载入文件时需要
};

}

#ifndef _MSC_VER
template<unsigned N>
static inline constexpr size_t alignSize(size_t nb)
{
    return (nb/N)*N + ((nb%N) ? N : 0);
}

class PlaceStringUnit: public details::PlaceStringUnitImpl
{
    enum {ePrefixSize=alignSize<eAlignment>(sizeof(details::PlaceStringUnitImpl)+sizeof(PlaceString))};
public:

    static constexpr ObjectIndex_t strSizeToFreeIndex(ObjectCount_t strSize)
    {
        return strSize/eAlignment;
    }

    static constexpr ObjectCount_t freeIndexToCapacity(ObjectIndex_t freeIndex)
    {
        return ePrefixSize+(freeIndex+1)*eAlignment;
    }

    static constexpr ObjectIndex_t capacityToFreeIndex(ObjectCount_t capacity)
    {
        return (capacity-ePrefixSize)/eAlignment-1;
    }

};

static_assert(sizeof(PlaceStringUnit)==sizeof(details::PlaceStringUnitImpl), "PlaceStringUnit: sizeof error.");
#endif







//=======================================================
//指定大小的类型封装
namespace details
{

template<unsigned SizeFixed, typename T, bool Equal>
class SizeFixedImplT;

template<unsigned SizeFixed, typename T>
class SizeFixedImplT<SizeFixed, T, true>
{
    static_assert(SizeFixed==sizeof(T) && SizeFixed%8==0, "SizeFixed need alignment");
public:
    typedef T Value;
    static_assert(sizeof(SizeFixedImplT::Value)==sizeof(T) && sizeof(SizeFixedImplT::Value)==SizeFixed, "error");
};

template<unsigned SizeFixed, typename T>
class SizeFixedImplT<SizeFixed, T, false>
{
    static_assert(SizeFixed>sizeof(T) && SizeFixed%8==0, "SizeFixed need alignment");
public:
    class Value: public T
    {
    public:
        template<typename... Args>
        Value(Args&&... args)
            :T(std::forward<Args&&>(args)...)
        {}

    private:
        char unused[SizeFixed-sizeof(T)]={};
    };

};

}

template<unsigned SizeFixed, typename T>
using SizeFixedT=typename details::SizeFixedImplT<SizeFixed, T, SizeFixed==sizeof(T)>::Value;




}


