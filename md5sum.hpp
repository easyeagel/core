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
#include"hash.hpp"

namespace core
{

class MD5Compute;

class MD5Sum: public HashSumT<MD5Sum, 16>
{
};

struct MD5Context
{
    uint32_t count[2];    /* message length in bits, lsw first */
    uint32_t abcd[4];     /* digest buffer */
    uint8_t  buf[64];     /* accumulate block */
};

class MD5Compute: public SumComputeT<MD5Compute, MD5Sum, MD5Context>
{
public:
    struct Call
    {
        static void init(MD5Context* ctx);
        static void update(MD5Context* ctx, const Byte_t* data, size_t nbyte);
        static void finish(MD5Context* ctx, Byte_t* digest);
    };
};

}


