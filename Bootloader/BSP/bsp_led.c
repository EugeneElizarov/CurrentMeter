#include "bsp_led.h"

/* Аппаратные настройки светодиода */
#define LED_PORT            GPIOB
#define LED_PIN             14U
#define LED_RCC_EN          RCC_AHBENR_GPIOBEN

/* Внутренние переменные состояния */
static BSP_LED_Mode_t current_mode = BSP_LED_OFF;
static uint8_t tick_counter = 0;
static uint8_t ticks_per_toggle = 0;
static uint8_t led_state = 0;

void BSP_LED_Init(void)
{
    /* 1. Настройка GPIO (PB14 как Push-Pull Output) */
    RCC->AHBENR |= LED_RCC_EN;

    LED_PORT->MODER &= ~(GPIO_MODER_MODER14);
    LED_PORT->MODER |= (1U << (LED_PIN * 2U));

    LED_PORT->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEEDR14);
    LED_PORT->ODR &= ~(1U << LED_PIN); /* Выключаем при старте */

    /* 2. Настройка SysTick для генерации прерывания каждые 125 мс
       При SYSCLK = 72 МГц: 72 000 000 / 8 = 9 000 000 тиков */
    SysTick->LOAD = 9000000U - 1U;
    SysTick->VAL = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | 
                    SysTick_CTRL_TICKINT_Msk | 
                    SysTick_CTRL_ENABLE_Msk;
}

void BSP_LED_Control(BSP_LED_Mode_t mode)
{
    current_mode = mode;
    tick_counter = 0U;
    ticks_per_toggle = 0U;

    /* Настройка параметров мигания в зависимости от режима */
    if (mode == BSP_LED_FLASH_1S)
    {
        ticks_per_toggle = 8U;  /* 8 * 125мс = 1000мс */
    }
    else if (mode == BSP_LED_FLASH_500MS)
    {
        ticks_per_toggle = 4U;  /* 4 * 125мс = 500мс */
    }
    else if (mode == BSP_LED_FLASH_250MS)
    {
        ticks_per_toggle = 2U;  /* 2 * 125мс = 250мс */
    }
    else if (mode == BSP_LED_FLASH_125MS)
    {
        ticks_per_toggle = 1U;  /* 1 * 125мс = 125мс */
    }

    /* Мгновенное применение состояния для статических режимов */
    if (mode == BSP_LED_ON)
    {
        LED_PORT->BSRR = (1U << LED_PIN);
        led_state = 1U;
    }
    else if (mode == BSP_LED_OFF)
    {
        LED_PORT->BSRR = (1U << (LED_PIN + 16U));
        led_state = 0U;
    }
}

/* ==============================================================================
 * Обработчик системного прерывания (Timebase 125мс)
 * ============================================================================== */

void SysTick_Handler(void)
{
    /* Обрабатываем только режимы мигания */
    if (ticks_per_toggle > 0U)
    {
        tick_counter++;

        if (tick_counter >= ticks_per_toggle)
        {
            tick_counter = 0U;
            led_state = !led_state;

            if (led_state)
            {
                LED_PORT->BSRR = (1U << LED_PIN);
            }
            else
            {
                LED_PORT->BSRR = (1U << (LED_PIN + 16U));
            }
        }
    }
}