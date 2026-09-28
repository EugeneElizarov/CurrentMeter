#ifndef USB_MSC_H
#define USB_MSC_H
#include <stdint.h>
#include <stdbool.h>
typedef struct { uint8_t bmRequestType; uint8_t bRequest; uint16_t wValue; uint16_t wIndex; uint16_t wLength; } USB_SetupPacket_t;
void USB_MSC_Init(void);
void USB_MSC_Reset(void);
void USB_MSC_Out(const uint8_t *data,uint16_t len);
void USB_MSC_InComplete(void);
bool USB_MSC_Control(const USB_SetupPacket_t *s,const uint8_t **data,uint16_t *len);
#endif
