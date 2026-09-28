// File: boot_crc32.c
#include "boot_crc32.h"
#include "stm32f3xx.h"

uint32_t BOOT_CalcCRC32(uint32_t addr, uint32_t length)
{
    RCC->AHBENR |= RCC_AHBENR_CRCEN;
    CRC->CR = CRC_CR_RESET;

    uint32_t words = length / 4U;
    const uint32_t *ptr = (const uint32_t *)addr;
    uint32_t i;

    for (i = 0; i < words; i++)
    {
        CRC->DR = ptr[i];
    }

    return CRC->DR;
}
