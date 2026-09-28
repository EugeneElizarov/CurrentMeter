#include "bsp_flash.h"
#include "stm32f3xx.h"

static bool flash_wait(void)
{
    while (FLASH->SR & FLASH_SR_BSY) {}
    return (FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPERR)) == 0U;
}

bool BSP_FLASH_Unlock(void)
{
    if ((FLASH->CR & FLASH_CR_LOCK) == 0U) return true;
    FLASH->KEYR = 0x45670123U;
    FLASH->KEYR = 0xCDEF89ABU;
    return (FLASH->CR & FLASH_CR_LOCK) == 0U;
}

void BSP_FLASH_Lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

bool BSP_FLASH_ErasePage(uint32_t addr)
{
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPERR;
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR = addr;
    FLASH->CR |= FLASH_CR_STRT;
    if (!flash_wait()) {
        FLASH->CR &= ~FLASH_CR_PER;
        return false;
    }
    FLASH->CR &= ~FLASH_CR_PER;
    FLASH->SR = FLASH_SR_EOP;
    return true;
}

bool BSP_FLASH_WriteHalfWord(uint32_t addr, uint16_t data)
{
    if (addr & 1U) return false;
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPERR;
    FLASH->CR |= FLASH_CR_PG;
    *(volatile uint16_t *)addr = data;
    if (!flash_wait()) {
        FLASH->CR &= ~FLASH_CR_PG;
        return false;
    }
    FLASH->CR &= ~FLASH_CR_PG;
    return *(volatile uint16_t *)addr == data;
}

bool BSP_FLASH_WriteWord(uint32_t addr, uint32_t data)
{
    return BSP_FLASH_WriteHalfWord(addr, (uint16_t)data) &&
           BSP_FLASH_WriteHalfWord(addr + 2U, (uint16_t)(data >> 16));
}
