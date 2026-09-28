// File: bsp_aes.h
#ifndef BSP_AES_H
#define BSP_AES_H

#include <stdint.h>

#define BSP_AES_BLOCK_SIZE  16U
#define BSP_AES_KEY_SIZE    16U

typedef struct
{
    uint8_t round_key[176];
} BSP_AES_Context_t;

void BSP_AES_Init(BSP_AES_Context_t *ctx, const uint8_t *key);
void BSP_AES_DecryptBlock(const BSP_AES_Context_t *ctx, const uint8_t *in, uint8_t *out);

#endif /* BSP_AES_H */