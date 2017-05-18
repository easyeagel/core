//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  hash.hpp
//
//   Description:  HASH 函数库
//
//       Version:  1.0
//       Created:  2014年01月10日 11时06分51秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================


#pragma once

#include<cstdint>

#include<array>
#include<vector>
#include<fstream>
#include"string.hpp"

namespace core
{

typedef uint32_t HashResult_t;

enum struct HashEnum: uint8_t
{
    eBKDRHash,
    eAPHash,
    eDJBHash,
    eJSHash,
    eRSHash,
    eSDBMHash,
    eDEKHash,
    eFNVHash,
    ePJWHash,
    eELFHash,

    //请在上面加入新值
    eHashCount
};

template<typename Int>
inline HashEnum hashEnumCast(Int val)
{
    assert(val>=0 && val<=static_cast<Int>(HashEnum::eHashCount));
    if(val>=static_cast<Int>(HashEnum::eHashCount))
        return HashEnum::eHashCount;
    return static_cast<HashEnum>(val);
}

HashResult_t callHash(HashEnum he, const char* str, const char* end);

template<typename Value>
inline HashResult_t callHash(HashEnum he, const Value* start, const Value* finish)
{
    const char* str=static_cast<const char*>(static_cast<const void*>(start));
    const char* end=static_cast<const char*>(static_cast<const void*>(finish));
    return callHash(he, str, end);
}

template<typename Int, typename Value>
inline HashResult_t callHash(Int he, const Value* start, const Value* finish)
{
    const char* str=static_cast<const char*>(static_cast<const void*>(start));
    const char* end=static_cast<const char*>(static_cast<const void*>(finish));
    return callHash(hashEnumCast(he), str, end);
}

uint64_t MurmurHash64B( const char* key, const char* const end, uint32_t seed=0xee6b27eb);

template<typename Obj, int NByte>
class HashSumT
{
public:
    typedef uint8_t  Byte_t;
    enum { eSize=NByte, eStringSize=eSize*2 };

    Byte_t* data()
    {
        return digest_.data();
    }

    const Byte_t* codeGet() const
    {
        return digest_.data();
    }

    const Byte_t* begin() const
    {
        return codeGet();
    }

    const Byte_t* end() const
    {
        return codeGet()+eSize;
    }

    std::string toString() const
    {
        std::string dest(eStringSize, '\0');
        StringHex::encode(
            reinterpret_cast<const char*>(begin()),
            reinterpret_cast<const char*>(end()),
            const_cast<char*>(dest.data())
        );
        for(auto& c: dest)
            c=std::tolower(c);
        return dest;
    }

    bool operator==(const Obj& other) const
    {
        return 0==compare(other);
    }

    bool operator!=(const Obj& other) const
    {
        return 0!=compare(other);
    }

    bool operator<(const Obj& other) const
    {
        return compare(other)<0;
    }

    bool operator<=(const Obj& other) const
    {
        return compare(other)<=0;
    }

    int compare(const Obj& other) const
    {
        return std::memcmp(codeGet(), other.codeGet(), sizeof(digest_));
    }

    static constexpr size_t sizeGet()
    {
        return eSize;
    }

    static constexpr size_t stringSizeGet()
    {
        return eStringSize;
    }

    bool empty() const
    {
        return std::all_of(std::begin(digest_), std::end(digest_), [](Byte_t c){ return c=='\0'; });
    }

    void fromData(const Byte_t* data)
    {
        std::memcpy(digest_, data, sizeof(digest_));
    }

    void fromString(const std::string& s)
    {
        assert(s.size()==eStringSize);
        if(s.size()!=eStringSize)
            return;

        char buf[eStringSize];
        for(size_t i=0; i<eStringSize; ++i)
        {
            buf[i]=std::toupper(s[i]);
            if(!(buf[i]>=0 && buf[i]<='f'))
                return;
        }

        StringHex::decode(buf, buf+sizeof(buf), reinterpret_cast<char*>(data()));
    }

    static Obj toObject(const std::string& s)
    {
        Obj ret;
        ret.fromString(s);
        return ret;
    }

private:
    std::array<Byte_t, NByte> digest_={{0}};
};

template<typename Obj, typename Sum, typename Context>
class SumComputeT
{
public:
    typedef typename Sum::Byte_t Byte_t;

    SumComputeT()
    {
        Obj::Call::init(&context_);
    }

    void reset()
    {
        Obj::Call::init(&context_);
    }

    void append(const Byte_t* data, size_t nbyte)
    {
        Obj::Call::update(&context_, data, nbyte);
    }

    void finish()
    {
        Obj::Call::finish(&context_, sum_.data());
    }

    const Sum& sumGet() const
    {
        return sum_;
    }

    static Sum streamDoit(const char* file)
    {
        std::ifstream in(file, std::ios::binary);
        if(!in)
            return Sum();

        Obj compute;
        Byte_t buffer[4*1024];
        while(in.read(reinterpret_cast<char*>(buffer), sizeof(buffer)))
            compute.append(buffer, static_cast<size_t>(in.gcount()));
        compute.append(buffer, static_cast<size_t>(in.gcount()));
        compute.finish();
        return compute.sumGet();
    }

    static Sum streamDoit(const std::vector<const char*>& files)
    {
        Obj compute;
        Byte_t buffer[4*1024];
        for(auto file: files)
        {
            std::ifstream in(file, std::ios::binary);
            if(!in)
                return Sum();

            while(in.read(reinterpret_cast<char*>(buffer), sizeof(buffer)))
                compute.append(buffer, static_cast<size_t>(in.gcount()));
            compute.append(buffer, static_cast<size_t>(in.gcount()));
        }

        compute.finish();
        return compute.sumGet();
    }

private:
    Sum sum_;
    Context context_;
};

}

