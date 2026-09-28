// File: usb_descriptors.c
#include "tusb.h"
#include "bootloader_config.h"

#define USBD_DESC_LEN (9 + 9 + 7 + 7)

tusb_desc_device_t const desc_device =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = 64,
    .idVendor           = BOOT_USB_VID,
    .idProduct          = BOOT_USB_PID,
    .bcdDevice          = BOOT_DEVICE_BCD,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

uint8_t const desc_configuration[] =
{
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, USBD_DESC_LEN, 0x00, 100),
    TUD_MSC_DESCRIPTOR(0, 0, 0x81, 0x02, 64)
};

static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_device_cb(void)
{
    return (uint16_t const*) &desc_device;
}

uint8_t const* tud_descriptor_configuration_cb(uint8_t index)
{
    (void) index;
    return desc_configuration;
}

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void) langid;
    uint8_t chr_count;

    if (index == 0)
    {
        memcpy(&_desc_str[1], "\x09\x04", 2);
        chr_count = 1;
    }
    else
    {
        const char *str = NULL;
        if (index == 1) str = BOOT_MANUFACTURER_STR;
        else if (index == 2) str = BOOT_PRODUCT_STR;
        else if (index == 3) str = BOOT_SERIAL_STR;
        else return NULL;

        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;

        for (uint8_t i = 0; i < chr_count; i++) _desc_str[1 + i] = str[i];
    }

    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);
    return _desc_str;
}