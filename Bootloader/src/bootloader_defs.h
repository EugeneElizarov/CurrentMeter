// File: bootloader_defs.h
#ifndef BOOTLOADER_DEFS_H
#define BOOTLOADER_DEFS_H

#include <stdint.h>
#include <stdbool.h>

#define BOOT_MAIN_APP_ADDR      0x08004000U
#define BOOT_MAIN_APP_SIZE      (48U * 1024U)
#define BOOT_BACKUP_APP_ADDR    0x08010000U
#define BOOT_BACKUP_APP_SIZE    (48U * 1024U)
#define BOOT_SETTINGS_ADDR      0x08018000U
#define BOOT_SETTINGS_SIZE      (8U * 1024U)

#define BOOT_FLASH_PAGE_SIZE    2048U
#define BOOT_SETTINGS_MAGIC     0xDEADBEEFU

typedef struct
{
    uint32_t magic;
    uint32_t main_crc;
    uint32_t backup_crc;
    uint32_t main_id;
    uint32_t backup_id;
} BOOT_Settings_t;

typedef struct
{
    uint32_t magic;
    uint32_t device_type;
    uint32_t firmware_id;
    uint32_t payload_size;
    uint32_t payload_crc32;
    uint32_t header_crc32;
    uint32_t reserved[2];
} BOOT_FW_Header_t;

typedef enum
{
    BOOT_MSC_STATE_IDLE = 0,
    BOOT_MSC_STATE_WAIT_HEADER,
    BOOT_MSC_STATE_RECEIVING,
    BOOT_MSC_STATE_REJECTED
} BOOT_MSC_State_t;

#endif /* BOOTLOADER_DEFS_H */