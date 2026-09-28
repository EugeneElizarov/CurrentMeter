#include "bsp_led.h"

#define LED_PORT            GPIOB
#define LED_PIN             14U
#define LED_RCC_EN          RCC_AHBENR_GPIOBEN
#define LED_SYSTICK_RELOAD  9000000U

static volatile uint8_t tick_counter;
static volatile uint8_t ticks_per_toggle;
static volatile uint8_t led_state;

void BSP_LED_Init(void)
{
  RCC->AHBENR |= LED_RCC_EN;

  LED_PORT->MODER &= ~GPIO_MODER_MODER14;
  LED_PORT->MODER |= 1UL << (LED_PIN * 2U);

  LED_PORT->OTYPER &= ~(1UL << LED_PIN);
  LED_PORT->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR14;
  LED_PORT->PUPDR &= ~GPIO_PUPDR_PUPDR14;
  LED_PORT->BSRR = 1UL << (LED_PIN + 16U);
  led_state = 0U;

  /* 72 MHz / 9,000,000 = 8 Hz => 125 ms. */
  SysTick->LOAD = LED_SYSTICK_RELOAD - 1U;
  SysTick->VAL = 0U;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
  SysTick_CTRL_TICKINT_Msk |
  SysTick_CTRL_ENABLE_Msk;
}

void BSP_LED_Control(BSP_LED_Mode_t mode)
{
  tick_counter = 0U;
  ticks_per_toggle = 0U;

  switch(mode)
  {
  case BSP_LED_FLASH_1S:
  ticks_per_toggle = 8U;
  break;
  case BSP_LED_FLASH_500MS:
  ticks_per_toggle = 4U;
  break;
  case BSP_LED_FLASH_250MS:
  ticks_per_toggle = 2U;
  break;
  case BSP_LED_FLASH_125MS:
  ticks_per_toggle = 1U;
  break;
  case BSP_LED_ON:
  LED_PORT->BSRR = 1UL << LED_PIN;
  led_state = 1U;
  break;
  case BSP_LED_OFF:
  default:
  LED_PORT->BSRR = 1UL << (LED_PIN + 16U);
  led_state = 0U;
  break;
  }
}

void SysTick_Handler(void)
{
  if(ticks_per_toggle == 0U)
  return;

  if(++tick_counter >= ticks_per_toggle)
  {
  tick_counter = 0U;
  led_state ^= 1U;

  if(led_state != 0U)
  LED_PORT->BSRR = 1UL << LED_PIN;
  else
  LED_PORT->BSRR = 1UL << (LED_PIN + 16U);
  }
}
