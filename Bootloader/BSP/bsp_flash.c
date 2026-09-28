#include "bsp_flash.h"
#include "bootloader_defs.h"
#include "stm32f3xx.h"

#define FLASH_TIMEOUT 10000000U

static bool flash_wait(void)
{
    for (uint32_t timeout = FLASH_TIMEOUT; timeout != 0U; --timeout)
    {
        if ((FLASH->SR & FLASH_SR_BSY) == 0U)
            return (FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPERR)) == 0U;
    }

    return false;
}

static bool flash_range_valid(uint32_t addr, uint32_t size)
{
    return addr >= BOOT_FLASH_BASE &&
           size <= BOOT_FLASH_END - addr;
}

bool BSP_FLASH_Unlock(void)
{
    if ((FLASH->CR & FLASH_CR_LOCK) == 0U)
        return true;

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
    if ((addr % BOOT_FLASH_PAGE_SIZE) != 0U ||
        !flash_range_valid(addr, BOOT_FLASH_PAGE_SIZE))
        return false;

    FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPERR;
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR = addr;
    FLASH->CR |= FLASH_CR_STRT;

    const bool ok = flash_wait();

    FLASH->CR &= ~FLASH_CR_PER;
    return ok;
}

bool BSP_FLASH_WriteHalfWord(uint32_t addr, uint16_t data)
{
    if ((addr & 1U) != 0U || !flash_range_valid(addr, sizeof(uint16_t)))
        return false;

    FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPERR;
    FLASH->CR |= FLASH_CR_PG;
    *(volatile uint16_t *)(uintptr_t)addr = data;

    const bool ok = flash_wait();

    FLASH->CR &= ~FLASH_CR_PG;
    return ok && *(volatile uint16_t *)(uintptr_t)addr == data;
}

bool BSP_FLASH_WriteWord(uint32_t addr, uint32_t data)
{
    return BSP_FLASH_WriteHalfWord(addr, (uint16_t)data) &&
           BSP_FLASH_WriteHalfWord(addr + 2U, (uint16_t)(data >> 16));
}
