// bsp_gpio.c
#include "bsp_gpio.h"
#include "bootloader_config.h"

void BSP_GPIO_Init(void)
{
  const uint32_t pin = BOOT_LED_PIN;
  const uint32_t mode_mask = 3UL << (pin * 2U);
  const uint32_t pin_mask = 1UL << pin;

  RCC->AHBENR |= BOOT_LED_RCC_EN;

  BOOT_LED_PORT->MODER &= ~mode_mask;
  BOOT_LED_PORT->MODER |= 1UL << (pin * 2U);

  BOOT_LED_PORT->OTYPER &= ~pin_mask;
  BOOT_LED_PORT->OSPEEDR &= ~mode_mask;
  BOOT_LED_PORT->PUPDR &= ~mode_mask;
  BOOT_LED_PORT->ODR &= ~pin_mask;
}

void BSP_GPIO_ToggleLed(void)
{
  BOOT_LED_PORT->ODR ^= 1UL << BOOT_LED_PIN;
}

void BSP_GPIO_SetLed(uint8_t state)
{
  if(state != 0U)
  BOOT_LED_PORT->BSRR = 1UL << BOOT_LED_PIN;
  else
  BOOT_LED_PORT->BSRR = 1UL << (BOOT_LED_PIN + 16U);
}
