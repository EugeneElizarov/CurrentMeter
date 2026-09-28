// bsp_usb_desc.h
#ifndef BSP_USB_DESC_H
#define BSP_USB_DESC_H

#include <stdint.h>

extern const uint8_t USBD_DeviceDescriptor[];
extern const uint8_t USBD_ConfigDescriptor[];
extern const uint8_t USBD_StringLangID[];
extern const uint8_t USBD_StringManufacturer[];
extern const uint8_t USBD_StringProduct[];
extern const uint8_t USBD_StringSerial[];

#endif /* BSP_USB_DESC_H */