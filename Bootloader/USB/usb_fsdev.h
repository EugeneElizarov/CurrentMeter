#ifndef USB_FSDEV_H
#define USB_FSDEV_H
#include <stdint.h>
#include <stdbool.h>
void USB_Device_Init(void);
void USB_Device_IRQHandler(void);
bool USB_EP_Send(uint8_t ep,const uint8_t *data,uint16_t len);
void USB_EP_Receive(uint8_t ep);
void USB_EP_Stall(uint8_t ep,bool in);
void USB_EP_ClearStall(uint8_t ep,bool in);
#endif
