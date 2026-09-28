// bsp_gpio.h
#ifndef BSP_GPIO_H
#define BSP_GPIO_H

#include <stdint.h>
#include "stm32f303xb.h"

void BSP_GPIO_Init(void);
void BSP_GPIO_ToggleLed(void);
void BSP_GPIO_SetLed(uint8_t state); // 1 = ON, 0 = OFF

#endif /* BSP_GPIO_H */