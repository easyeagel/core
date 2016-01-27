//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  tinyaes.cpp
//
//   Description:  tinyAES128 简单封装 实现
//
//       Version:  1.0
//       Created:  2014年12月05日 14时15分59秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<cstring>

#include"tinyaes.hpp"
#include"tinyAES128/aes.h"

namespace core
{

void TinyAES128::encryptStep(const uint8_t* key, const uint8_t* input, uint8_t *output)
{
    AES128_ECB_encrypt(&contex_, input, key, output);
}

void TinyAES128::decryptStep(const uint8_t* key, const uint8_t* input, uint8_t *output)
{
    AES128_ECB_decrypt(&contex_, input, key, output);
}

void TinyAES128::encrypt(const uint8_t* key, const uint8_t* input, size_t sz, uint8_t* output)
{
    IntCode::encode(reinterpret_cast<char*>(output), static_cast<uint32_t>(sz));
    output += 4;

    for(;;sz -= 16, input += 16, output += 16)
    {
        if(sz>=16)
        {
            encryptStep(key, input, output);
            continue;
        }

        if(sz==0)
            return;

        uint8_t block[16]={};
        std::memcpy(block, input, sz);
        encryptStep(key, block, output);
        return;
    }
}

void TinyAES128::decrypt(const uint8_t* key, const uint8_t* input, uint8_t* output)
{
    uint32_t destSize=IntCode::decode<uint32_t>(reinterpret_cast<const char*>(input));
    const size_t sz=encryptSizeGet(destSize);
    const auto inputEnd=input+sz;
    input += 4;

    for(; input<inputEnd; input += 16, output += 16, destSize -= 16)
    {
        if(destSize>=16)
        {
            decryptStep(key, input, output);
            continue;
        }

        if(destSize==0)
            return;

        uint8_t block[16];
        decryptStep(key, input, block);
        std::memcpy(output, block, destSize);
        return;
    }
}

}  // namespace mr

