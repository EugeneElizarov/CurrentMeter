#ifndef BSP_LED_H
#define BSP_LED_H

#include <stdint.h>
#include "stm32f3xx.h"

/* ==============================================================================
 * Типы и перечисления
 * ============================================================================== */

typedef enum
{
    BSP_LED_OFF = 0,
    BSP_LED_FLASH_1S,
    BSP_LED_FLASH_500MS,
    BSP_LED_FLASH_250MS,
    BSP_LED_FLASH_125MS,
    BSP_LED_ON
} BSP_LED_Mode_t;

/* ==============================================================================
 * Прототипы функций API
 * ============================================================================== */

/**
 * @brief Инициализация GPIO светодиода (PB14) и системного таймера SysTick.
 */
void BSP_LED_Init(void);

/**
 * @brief Управление режимом работы светодиода.
 * @param mode Желаемый режим (OFF, FLASH_1S, FLASH_500MS, FLASH_250MS, FLASH_125MS, ON).
 */
void BSP_LED_Control(BSP_LED_Mode_t mode);

#endif /* BSP_LED_H */
