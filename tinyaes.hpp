//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  tinyaes.hpp
//
//   Description:  tinyAES128 简单封装
//
//       Version:  1.0
//       Created:  2014年12月05日 14时00分04秒
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
#include"tinyAES128/aes.h"

namespace core
{

class TinyAES128
{
public:
    void encryptStep(const uint8_t* key, const uint8_t* input, uint8_t *output);
    void decryptStep(const uint8_t* key, const uint8_t* input, uint8_t *output);

    static constexpr size_t encryptSizeGet(size_t in)
    {
        return 4 + 16*(in/16 + ((in%16)==0 ? 0 : 1) );
    }
    void encrypt(const uint8_t* key, const uint8_t* input, size_t sz, uint8_t* output);

    template<typename VK, typename VI, typename VO>
    void encrypt(const VK* key, const VI* input, size_t sz, VO* output)
    {
        return encrypt(reinterpret_cast<const uint8_t*>(key),
            reinterpret_cast<const uint8_t*>(input), sz,
            reinterpret_cast<uint8_t*>(output)
        );
    }

    template<typename V>
    static size_t decryptSizeGet(const V* input)
    {
        return decryptSizeGet(reinterpret_cast<const uint8_t*>(input));
    }

    static size_t decryptSizeGet(const uint8_t* input)
    {
        return IntCode::decode<uint32_t>(reinterpret_cast<const char*>(input));
    }

    static size_t decryptBlockSizeGet(const uint8_t* input)
    {
        return encryptSizeGet(IntCode::decode<uint32_t>(reinterpret_cast<const char*>(input)));
    }

    void decrypt(const uint8_t* key, const uint8_t* input, uint8_t* output);

    template<typename VK, typename VI, typename VO>
    void decrypt(const VK* key, const VI* input, VO* output)
    {
        return decrypt(reinterpret_cast<const uint8_t*>(key),
            reinterpret_cast<const uint8_t*>(input),
            reinterpret_cast<uint8_t*>(output)
        );
    }
private:
    AES128Context contex_;
};

}

