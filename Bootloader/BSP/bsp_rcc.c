// File: bsp_rcc.c
#include "bsp_rcc.h"
#include "stm32f3xx.h"
#include "system_stm32f3xx.h"

#define RCC_CFGR_USBPRE_BIT       (1UL << 22)
#define RCC_APB1ENR_USBEN_BIT     (1UL << 23)
#define GPIOA_USB_PINS_MASK       ((3UL << (11U * 2U)) | (3UL << (12U * 2U)))
#define GPIOA_USB_AF_MASK         ((0xFUL << ((11U - 8U) * 4U)) | (0xFUL << ((12U - 8U) * 4U)))
#define GPIOA_USB_AF              14U

void BSP_RCC_Init(void)
{
    FLASH->ACR &= ~FLASH_ACR_LATENCY;
    FLASH->ACR |= FLASH_ACR_LATENCY_2;

    RCC->CR |= RCC_CR_HSEON;
    while ((RCC->CR & RCC_CR_HSERDY) == 0U) {}

    RCC->CR &= ~RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) != 0U) {}

    /*
     * HSE = 12 MHz, PLL = 72 MHz.
     * STM32F303xB/C derives the USB FS clock from PLL through USBPRE:
     * USBPRE = 0 selects PLL / 1.5 = 48 MHz.
     */
    RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL | RCC_CFGR_USBPRE_BIT);
    RCC->CFGR |= RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR_PLLMUL6;

    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0U) {}

    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1;

    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}

    /*
     * STM32F303xB/C USB pins are PA11/PA12, AF14.
     * USB is an "additional function" on these pins, but the AF14
     * selection is required for the xB/xC line.
     */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER &= ~GPIOA_USB_PINS_MASK;
    GPIOA->MODER |= (2UL << (11U * 2U)) | (2UL << (12U * 2U));
    GPIOA->OTYPER &= ~((1UL << 11U) | (1UL << 12U));
    GPIOA->OSPEEDR |= (3UL << (11U * 2U)) | (3UL << (12U * 2U));
    GPIOA->PUPDR &= ~GPIOA_USB_PINS_MASK;
    GPIOA->AFR[1] &= ~GPIOA_USB_AF_MASK;
    GPIOA->AFR[1] |= (GPIOA_USB_AF << ((11U - 8U) * 4U)) |
                     (GPIOA_USB_AF << ((12U - 8U) * 4U));

    RCC->APB1ENR |= RCC_APB1ENR_USBEN_BIT;

    SystemCoreClock = BSP_RCC_SYSCLK_FREQ_HZ;
}

uint32_t BSP_RCC_GetSystemCoreClock(void)
{
    return BSP_RCC_SYSCLK_FREQ_HZ;
}
