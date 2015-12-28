//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  hash.cpp
//
//   Description:  hash函数库
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

#include<stdlib.h>
#include<core/macro.hpp>

#include"hash.hpp"

namespace mr
{

static inline HashResult_t SDBMHash(const char *str, const char* const end)
{
    HashResult_t hash = 0;
    for(; str<end; ++str)
    {
        // equivalent to: hash = 65599*hash + (*str);
        hash = static_cast<HashResult_t>(*str) + (hash << 6) + (hash << 16) - hash;
    }

    return (hash & 0x7FFFFFFF);
}

static inline HashResult_t RSHash(const char *str, const char* const end)
{
    HashResult_t b = 378551;
    HashResult_t a = 63689;
    HashResult_t hash = 0;

    for(; str<end; ++str)
    {
        hash = hash * a + static_cast<HashResult_t>(*str);
        a *= b;
    }

    return (hash & 0x7FFFFFFF);
}

static inline HashResult_t JSHash(const char *str, const char* const end)
{
    HashResult_t hash = 1315423911;
    for(; str<end; ++str)
    {
        hash ^= ((hash << 5) + static_cast<HashResult_t>(*str) + (hash >> 2));
    }

    return (hash & 0x7FFFFFFF);
}

static inline HashResult_t PJWHash(const char *str, const char* const end)
{
    HashResult_t BitsInUnignedInt = static_cast<HashResult_t>(sizeof(HashResult_t) * 8);
    HashResult_t ThreeQuarters    = static_cast<HashResult_t>((BitsInUnignedInt  * 3) / 4);
    HashResult_t OneEighth        = static_cast<HashResult_t>(BitsInUnignedInt / 8);
    HashResult_t HighBits         = static_cast<HashResult_t>(0xFFFFFFFF) << (BitsInUnignedInt - OneEighth);
    HashResult_t hash             = 0;
    HashResult_t test             = 0;

    for(; str<end; ++str)
    {
        hash = (hash << OneEighth) + static_cast<HashResult_t>(*str);
        if ((test = hash & HighBits) != 0)
        {
            hash = ((hash ^ (test >> ThreeQuarters)) & (~HighBits));
        }
    }

    return (hash & 0x7FFFFFFF);
}

static inline HashResult_t ELFHash(const char *str, const char* const end)
{
    HashResult_t hash = 0;
    HashResult_t x    = 0;

    for(; str<end; ++str)
    {
        hash = (hash << 4) + static_cast<HashResult_t>(*str);
        if ((x = hash & 0xF0000000L) != 0)
        {
            hash ^= (x >> 24);
            hash &= ~x;
        }
    }

    return (hash & 0x7FFFFFFF);
}

static inline HashResult_t BKDRHash(const char *str, const char* const end)
{
    HashResult_t seed = 131; // 31 131 1313 13131 131313 etc..
    HashResult_t hash = 0;

    for(; str<end; ++str)
    {
        hash = hash * seed + static_cast<HashResult_t>(*str);
    }

    return (hash & 0x7FFFFFFF);
}

static inline HashResult_t DJBHash(const char *str, const char* const end)
{
    HashResult_t hash = 5381;

    for(; str<end; ++str)
    {
        hash += (hash << 5) + static_cast<HashResult_t>(*str);
    }

    return (hash & 0x7FFFFFFF);
}

static inline HashResult_t APHash(const char *str, const char* const end)
{
    HashResult_t hash = 0;
    for (unsigned i=0; str<end; ++i, ++str)
    {
        if ((i & 1) == 0)
        {
            hash ^= ((hash << 7) ^ static_cast<HashResult_t>(*str) ^ (hash >> 3));
        }
        else
        {
            hash ^= (~((hash << 11) ^ static_cast<HashResult_t>(*str) ^ (hash >> 5)));
        }
    }

    return (hash & 0x7FFFFFFF);
}

static inline HashResult_t DEKHash(const char* str, const char* const end)
{
    HashResult_t hash = static_cast<HashResult_t>(end-str);
    for(; str<end; ++str)
    {
        hash = ((hash << 5) ^ (hash >> 27)) ^ static_cast<HashResult_t>(*str);
    }
    return hash;
}

static inline HashResult_t FNVHash(const char* str, const char* const end)
{
    const HashResult_t fnv_prime = 0x811C9DC5;
    HashResult_t hash      = 0;
    for(; str<end; ++str)
    {
        hash *= fnv_prime;
        hash ^= static_cast<HashResult_t>(*str);
    }
    return hash;
}

uint64_t MurmurHash64B( const char* key, const char* const end, uint32_t seed)
{
    int len=end-key;
    const int r = 24;
    const uint32_t m = 0x5bd1e995;

    uint32_t h1 = seed ^ len;
    uint32_t h2 = 0;

    const uint32_t* data = reinterpret_cast<const uint32_t*>(key);

    while(len >= 8)
    {
        uint32_t k1 = *data++;
        k1 *= m; k1 ^= k1 >> r; k1 *= m;
        h1 *= m; h1 ^= k1;
        len -= 4;

        uint32_t k2 = *data++;
        k2 *= m; k2 ^= k2 >> r; k2 *= m;
        h2 *= m; h2 ^= k2;
        len -= 4;
    }

    if(len >= 4)
    {
        uint32_t k1 = *data++;
        k1 *= m; k1 ^= k1 >> r; k1 *= m;
        h1 *= m; h1 ^= k1;
        len -= 4;
    }

    switch(len)
    {
        case 3: h2 ^= reinterpret_cast<const uint8_t*>(data)[2] << 16;
        case 2: h2 ^= reinterpret_cast<const uint8_t*>(data)[1] << 8;
        case 1: h2 ^= reinterpret_cast<const uint8_t*>(data)[0];
                h2 *= m;
    };

    h1 ^= h2 >> 18; h1 *= m;
    h2 ^= h1 >> 22; h2 *= m;
    h1 ^= h2 >> 17; h1 *= m;
    h2 ^= h1 >> 19; h2 *= m;

    uint64_t h = h1;

    h = (h << 32) | h2;

    return h;
}

HashResult_t callHash(HashEnum he, const char* str, const char* end)
{
    switch (he)
    {
        case HashEnum::eBKDRHash:
            return BKDRHash(str, end);
        case HashEnum::eAPHash:
            return APHash(str, end);
        case HashEnum::eDJBHash:
            return DJBHash(str, end);
        case HashEnum::eJSHash:
            return JSHash(str, end);
        case HashEnum::eRSHash:
            return RSHash(str, end);
        case HashEnum::eSDBMHash:
            return SDBMHash(str, end);
        case HashEnum::eDEKHash:
            return DEKHash(str, end);
        case HashEnum::eFNVHash:
            return FNVHash(str, end);
        case HashEnum::ePJWHash:
            return PJWHash(str, end);
        case HashEnum::eELFHash:
            return ELFHash(str, end);
        case HashEnum::eHashCount:
        default:
            GMacroAbort("enum out of range");
            return 0;
    };
}

}

