#ifndef BOOTLOADER_CONFIG_H
#define BOOTLOADER_CONFIG_H
#include <stdint.h>
#define BOOT_USB_VID 0x0483U
#define BOOT_USB_PID 0x5740U
#define BOOT_MANUFACTURER_STR "CurrentMeter"
#define BOOT_PRODUCT_STR "CurrentMeter Bootloader"
#define BOOT_SERIAL_STR "000000000001"
#define BOOT_DEVICE_BCD 0x0100U
/* Protocol constants. Replace these test values with the production key/IV before encrypted images are released. */
#define BOOT_AES_KEY {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
#define BOOT_AES_IV  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
#endif
