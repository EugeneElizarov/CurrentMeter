#include "bsp_usb_msc.h"
#include "usb_fsdev.h"
#include "usb_msc.h"
void BSP_USB_MSC_Init(void){USB_MSC_Init();USB_Device_Init();}
void BSP_USB_MSC_Task(void){}
