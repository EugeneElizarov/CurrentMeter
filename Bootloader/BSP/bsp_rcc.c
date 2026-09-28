// File: bsp_rcc.c
#include "bsp_rcc.h"
#include "stm32f3xx.h"

void BSP_RCC_Init(void)
{
    FLASH->ACR |= FLASH_ACR_LATENCY_2;

    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    RCC->CR &= ~RCC_CR_PLLON;
    while (RCC->CR & RCC_CR_PLLRDY);

    /* HSE = 12 MHz -> PLL = 72 MHz.
       On STM32F303 the USB FS clock is derived from the PLL clock;
       there is no RCC_CFGR_USBPRES field in this device header. */
    RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL);
    RCC->CFGR |= RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR_PLLMUL6;

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1;

    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}

uint32_t BSP_RCC_GetSystemCoreClock(void)
{
    return BSP_RCC_SYSCLK_FREQ_HZ;
}
