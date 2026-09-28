// bsp_gpio.c
#include "bsp_gpio.h"
#include "bootloader_config.h"

void BSP_GPIO_Init(void)
{
    /* Включаем тактирование порта B */
    RCC->AHBENR |= BOOT_LED_RCC_EN;

    /* Настраиваем PB14 как выход (Push-Pull) */
    /* Очищаем биты MODER для пина 14 */
    BOOT_LED_PORT->MODER &= ~(GPIO_MODER_MODER14);
    /* Устанавливаем режим Output (01) */
    BOOT_LED_PORT->MODER |= (1U << (BOOT_LED_PIN * 2));

    /* Скорость Low (для светодиода достаточно) */
    BOOT_LED_PORT->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEEDR14);
    
    /* Выключаем светодиод при старте (сбрасываем бит в ODR) */
    BOOT_LED_PORT->ODR &= ~(1U << BOOT_LED_PIN);
}

void BSP_GPIO_ToggleLed(void)
{
    BOOT_LED_PORT->ODR ^= (1U << BOOT_LED_PIN);
}

void BSP_GPIO_SetLed(uint8_t state)
{
    if (state)
    {
        BOOT_LED_PORT->BSRR = (1U << BOOT_LED_PIN);
    }
    else
    {
        BOOT_LED_PORT->BSRR = (1U << (BOOT_LED_PIN + 16U));
    }
}