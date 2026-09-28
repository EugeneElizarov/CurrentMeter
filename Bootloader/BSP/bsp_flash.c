// File: bsp_flash.c
#include "bsp_flash.h"
#include "stm32f303xb.h"

bool BSP_FLASH_Unlock(void)
{
    if ((FLASH->CR & FLASH_CR_LOCK) == 0) return true;

    FLASH->KEYR = 0x45670123U;
    FLASH->KEYR = 0xCDEF89ABU;

    return ((FLASH->CR & FLASH_CR_LOCK) == 0);
}

void BSP_FLASH_Lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

bool BSP_FLASH_ErasePage(uint32_t addr)
{
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR = addr;
    FLASH->CR |= FLASH_CR_STRT;

    while (FLASH->SR & FLASH_SR_BSY);
    FLASH->CR &= ~FLASH_CR_PER;

    return ((FLASH->SR & FLASH_SR_EOP) != 0);
}

bool BSP_FLASH_WriteWord(uint32_t addr, uint32_t data)
{
    FLASH->CR |= FLASH_CR_PG;
    *(volatile uint32_t *)addr = data;
    while (FLASH->SR & FLASH_SR_BSY);
    FLASH->CR &= ~FLASH_CR_PG;
    return true;
}