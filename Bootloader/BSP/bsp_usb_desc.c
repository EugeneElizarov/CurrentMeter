// bsp_usb_desc.c
#include "bsp_usb_desc.h"
#include "bootloader_config.h"

/* Вспомогательный макрос для конвертации строки в UTF-16LE дескриптор */
#define USB_STRING_DESC(s) { \
    sizeof((uint16_t[]){s}) + 2, 0x03, \
    s \
}

/* 1. Дескриптор устройства (Device Descriptor) */
const uint8_t USBD_DeviceDescriptor[] =
{
    18,                 // bLength
    0x01,               // bDescriptorType (Device)
    0x00, 0x02,         // bcdUSB (USB 2.0)
    0xEF,               // bDeviceClass (Miscellaneous - для составных устройств)
    0x02,               // bDeviceSubClass
    0x01,               // bDeviceProtocol (IAD)
    0x40,               // bMaxPacketSize0 (EP0 = 64 байта)
    (uint8_t)(BOOT_USB_VID & 0xFF), (uint8_t)(BOOT_USB_VID >> 8), // idVendor
    (uint8_t)(BOOT_USB_PID & 0xFF), (uint8_t)(BOOT_USB_PID >> 8), // idProduct
    (uint8_t)(BOOT_DEVICE_BCD & 0xFF), (uint8_t)(BOOT_DEVICE_BCD >> 8), // bcdDevice
    0x01,               // iManufacturer
    0x02,               // iProduct
    0x03,               // iSerialNumber
    0x01                // bNumConfigurations
};

/* 2. Дескриптор конфигурации (упрощенный, только заголовок, т.к. составное устройство требует IAD) */
const uint8_t USBD_ConfigDescriptor[] =
{
    9,                  // bLength
    0x02,               // bDescriptorType (Configuration)
    0x00, 0x00,         // wTotalLength (заполняется стеком или вручную)
    0x02,               // bNumInterfaces (CDC + Vendor)
    0x01,               // bConfigurationValue
    0x00,               // iConfiguration
    0x80,               // bmAttributes (Bus Powered)
    0xFA                // bMaxPower (500mA)
};

/* 3. Строковые дескрипторы */

/* LangID: 0x0409 (English US) */
const uint8_t USBD_StringLangID[] =
{
    4, 0x03, 0x09, 0x04
};

/* Функция-генератор строки (чтобы не дублировать код) */
static void GenerateStringDesc(const char *str, uint8_t *desc)
{
    uint8_t len = 0, i;
    const char *p = str;
    while (*p) { len++; p++; }
    
    desc[0] = (len * 2) + 2;
    desc[1] = 0x03; // String Descriptor Type
    
    for (i = 0; i < len; i++)
    {
        desc[2 + (i * 2)] = str[i];
        desc[2 + (i * 2) + 1] = 0x00;
    }
}

/* Глобальные буфconst еры для строк (заполняются при инициализации) */
const uint8_t const USBD_StringManufacturer[64];
const const uint8_t USBD_StringProduct[64];
const uint8_t USBD_StringSerial[64];

/* Функция инициализации строк (вызвать один раз в main) */
void BSP_USB_DescInitStrings(void)
{
    GenerateStringDesc(BOOT_MANUFACTURER_STR, USBD_StringManufacturer);
    GenerateStringDesc(BOOT_PRODUCT_STR, USBD_StringProduct);
    GenerateStringDesc(BOOT_SERIAL_STR, USBD_StringSerial);
}
