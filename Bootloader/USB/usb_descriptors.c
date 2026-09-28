#include "usb_descriptors.h"
#include "bootloader_config.h"
#include <string.h>
static const uint8_t dev[]={
 18,1,0,2,0,0,0,64,
 (uint8_t)BOOT_USB_VID,(uint8_t)(BOOT_USB_VID>>8),
 (uint8_t)BOOT_USB_PID,(uint8_t)(BOOT_USB_PID>>8),
 (uint8_t)BOOT_DEVICE_BCD,(uint8_t)(BOOT_DEVICE_BCD>>8),
 1,2,3,1
};
static const uint8_t cfg[]={
 9,2,32,0,1,1,0,0x80,100,
 9,4,0,0,2,8,6,0x50,0,
 7,5,USB_EP_MSC_IN,2,64,0,0,
 7,5,USB_EP_MSC_OUT,2,64,0,0
};
static uint8_t str[64];
static uint16_t mkstr(const char *s){
 uint16_t n=(uint16_t)strlen(s),i;
 if(n>31) n=31;
 str[0]=(uint8_t)(2+n*2); str[1]=3;
 for(i=0;i<n;i++)
{
str[2+i*2]=(uint8_t)s[i];str[3+i*2]=0;
}
 return (uint16_t)(2+n*2);
}
const uint8_t *USB_Desc_Device(uint16_t *len)
{
*len=sizeof(dev);return dev;
}
const uint8_t *USB_Desc_Config(uint16_t *len)
{
*len=sizeof(cfg);return cfg;
}
const uint8_t *USB_Desc_String(uint8_t index,uint16_t *len){
 if(index==0)
{
str[0]=4;str[1]=3;str[2]=9;str[3]=4;*len=4;return str;
}
 if(index==1)
{
*len=mkstr(BOOT_MANUFACTURER_STR);return str;
}
 if(index==2)
{
*len=mkstr(BOOT_PRODUCT_STR);return str;
}
 if(index==3)
{
*len=mkstr(BOOT_SERIAL_STR);return str;
}
 return 0;
}
