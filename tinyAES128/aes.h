#ifndef _AES_H_
#define _AES_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct AES128Context
{
    const uint8_t* in;
    const uint8_t* key;

    uint8_t* out;

    uint8_t state[4][4];
    uint8_t RoundKey[176];
}AES128Context;

void AES128_ECB_encrypt(AES128Context* ctx, const uint8_t* input, const uint8_t* key, uint8_t *output);
void AES128_ECB_decrypt(AES128Context* ctx, const uint8_t* input, const uint8_t* key, uint8_t *output);

#ifdef __cplusplus
}
#endif


#endif //_AES_H_
