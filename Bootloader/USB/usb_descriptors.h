#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H
#include <stdint.h>
#define USB_EP_MSC_IN 0x81U
#define USB_EP_MSC_OUT 0x02U
const uint8_t *USB_Desc_Device(uint16_t *len);
const uint8_t *USB_Desc_Config(uint16_t *len);
const uint8_t *USB_Desc_String(uint8_t index,uint16_t *len);
#endif
