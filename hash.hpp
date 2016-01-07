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

namespace mr
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

}

