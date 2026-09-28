#ifndef BOOTLOADER_CONFIG_H
#define BOOTLOADER_CONFIG_H

#include <stdint.h>

/* ==============================================================================
 * USB Идентификация
 * ============================================================================== */

/* VID от STMicroelectronics (официальный для CDC/DFU устройств) */
#define BOOT_USB_VID            0x0483U

/* PID (выбран как свободный из диапазона STM32 Custom/CDC устройств) */
#define BOOT_USB_PID            0x5740U

/* Строковые дескрипторы (в формате UTF-16LE, но макросы ниже упростят это) */
#define BOOT_MANUFACTURER_STR   "STMicroelectronics"
#define BOOT_PRODUCT_STR        "Bracelet Bootloader"
#define BOOT_SERIAL_STR         "000000000001"

/* Версия устройства (BCD формат, например 1.0.0 = 0x0100) */
#define BOOT_DEVICE_BCD         0x0100U

/* ==============================================================================
 * Аппаратная конфигурация загрузчика
 * ============================================================================== */

/* Светодиод индикации режимов */
#define BOOT_LED_PORT           GPIOB
#define BOOT_LED_PIN            14U
#define BOOT_LED_RCC_EN         RCC_AHBENR_GPIOBEN

/* Режимы работы загрузчика (для логики моргания) */
typedef enum
{
    BOOT_MODE_IDLE = 0,         // Ожидание подключения / штатный запуск
    BOOT_MODE_UPDATING,         // Идет запись новой прошивки
    BOOT_MODE_ERROR             // Ошибка (CRC, запись)
} Boot_Mode_t;

#endif /* BOOTLOADER_CONFIG_H */