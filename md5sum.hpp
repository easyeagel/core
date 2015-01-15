//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  md5sum.hpp
//
//   Description:  md5 算法接口
//
//       Version:  1.0
//       Created:  2014年05月09日 10时41分34秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================


#pragma once

#include<cctype>
#include<cstring>

#include<vector>
#include<string>
#include<memory>
#include<algorithm>

#include"msvc.hpp"
#include"string.hpp"
#include"typedef.hpp"

namespace core
{

class MD5Compute;

class MD5Sum
{
    friend class MD5Compute;
public:
#ifndef _MSC_VER
    MD5Sum()
    {
        std::memset(digest_, 0, sizeof(digest_));
    }
#endif

    typedef uint8_t  Byte_t;
    const Byte_t* codeGet() const
    {
        return digest_;
    }

    const Byte_t* begin() const
    {
        return std::begin(digest_);
    }

    const Byte_t* end() const
    {
        return std::end(digest_);
    }

    std::string toString() const
    {
        std::string dest(32, '\0');
        StringHex::encode(
            reinterpret_cast<const char*>(begin()),
            reinterpret_cast<const char*>(end()),
            const_cast<char*>(dest.data())
        );
        for(auto& c: dest)
            c=std::tolower(c);
        return std::move(dest);
    }

    bool operator==(const MD5Sum& other) const
    {
        return 0==compare(other);
    }

    bool operator!=(const MD5Sum& other) const
    {
        return 0!=compare(other);
    }

    bool operator<(const MD5Sum& other) const
    {
        return compare(other)<0;
    }

    bool operator<=(const MD5Sum& other) const
    {
        return compare(other)<=0;
    }

    int compare(const MD5Sum& other) const
    {
        return std::memcmp(digest_, other.digest_, sizeof(digest_));
    }

    static constexpr size_t sizeGet()
    {
        return 16;
    }

    static constexpr size_t stringSizeGet()
    {
        return 16*2;
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
        assert(s.size()==32);

        char buf[32];
        for(size_t i=0; i<32; ++i)
            buf[i]=std::toupper(s[i]);

        StringHex::decode(buf, buf+sizeof(buf), reinterpret_cast<char*>(digest_));
    }

private:
#ifndef _MSC_VER
    Byte_t digest_[16]={0};
#else
    Byte_t digest_[16];
#endif
};

class MD5Compute
{
public:
    typedef uint8_t  Byte_t;
    typedef uint32_t Word_t;
    struct Status
    {
        Word_t count[2];    /* message length in bits, lsw first */
        Word_t abcd[4];     /* digest buffer */
        Byte_t buf[64];     /* accumulate block */
    };

    MD5Compute();
    void reset();
    void append(const Byte_t* data, size_t nbyte);
    void finish();

    const MD5Sum& sumGet() const
    {
        return sum_;
    }

    static MD5Sum streamDoit(const char* file);
    static MD5Sum streamDoit(const std::vector<const char*>& files);

private:
    Status status_;
    MD5Sum sum_;
};

}


