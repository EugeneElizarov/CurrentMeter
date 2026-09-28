#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#define CFG_TUSB_MCU            OPT_MCU_STM32F3

/* STM32F303 USB FS device register and packet-memory base addresses.
 * The local minimal CMSIS device header does not currently declare these. */
#ifndef USB_BASE
#define USB_BASE                0x40005C00UL
#endif
#ifndef USB_PMAADDR
#define USB_PMAADDR             0x40006000UL
#endif
#define CFG_TUSB_OS             OPT_OS_NONE
#define CFG_TUSB_RHPORT0_MODE   (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN      __attribute__((aligned(4)))
#define CFG_TUD_ENABLED         1
#define CFG_TUD_MSC             1
#define CFG_TUD_CDC             0
#define CFG_TUD_HID             0
#define CFG_TUD_VENDOR          0
#define CFG_TUD_ENDPOINT0_SIZE  64
#define CFG_TUD_MSC_BUFSIZE     512
#define CFG_TUD_TASK_QUEUE_SZ   16
#define CFG_TUSB_DEBUG          0

#endif
