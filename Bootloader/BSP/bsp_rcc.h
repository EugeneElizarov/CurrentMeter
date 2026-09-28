// File: bsp_rcc.h
#ifndef BSP_RCC_H
#define BSP_RCC_H

#include <stdint.h>

#define BSP_RCC_HSE_FREQ_HZ         12000000U
#define BSP_RCC_SYSCLK_FREQ_HZ      72000000U

void BSP_RCC_Init(void);
uint32_t BSP_RCC_GetSystemCoreClock(void);

#endif /* BSP_RCC_H */