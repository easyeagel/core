//  Copyright [2017] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  sha2.hpp
//
//   Description: sha2 算法包装
//
//       Version:  1.0
//       Created:  2017年05月18日 15时39分46秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once
#include"hash.hpp"
#include"sqlite/sha2.h"

namespace core
{

class Sha256Sum: public HashSumT<Sha256Sum, SHA256_DIGEST_SIZE>
{

};

typedef sha256_ctx Sha256Context;

class Sha256Compute: public SumComputeT<Sha256Compute, Sha256Sum, Sha256Context>
{
public:
    struct Call
    {
        static void init(Sha256Context* ctx)
        {
            sha256_init(ctx);
        }

        static void update(Sha256Context* ctx, const Byte_t* data, size_t nbyte)
        {
            sha256_update(ctx, data, nbyte);
        }

        static void finish(Sha256Context* ctx, Byte_t* digest)
        {
            sha256_final(ctx, digest);
        }
    };
};


}

