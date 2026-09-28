#ifndef BOOTLOADER_DEFS_H
#define BOOTLOADER_DEFS_H

#include <stdint.h>
#include <stdbool.h>

#define BOOT_FLASH_BASE         0x08000000U
#define BOOT_FLASH_SIZE         (128U * 1024U)
#define BOOT_FLASH_END          (BOOT_FLASH_BASE + BOOT_FLASH_SIZE)
#define BOOT_FLASH_PAGE_SIZE    2048U

#define BOOTLOADER_SIZE         (16U * 1024U)
#define BOOT_MAIN_APP_ADDR      0x08004000U
#define BOOT_MAIN_APP_SIZE      (54U * 1024U)
#define BOOT_BACKUP_APP_ADDR    0x08011800U
#define BOOT_BACKUP_APP_SIZE    (54U * 1024U)

#define BOOT_SETTINGS1_ADDR     0x0801F000U
#define BOOT_SETTINGS2_ADDR     0x0801F800U
#define BOOT_SETTINGS_SIZE      2048U

#define BOOT_FW_MAGIC           0x46574D31U
#define BOOT_SETTINGS_MAGIC     0x53455431U
#define DEVICE_TYPE_CURRENTMETER 0x00000001U

typedef struct
{
  uint8_t  version;
  uint8_t  subversion;
  uint16_t cpu_id;
  uint32_t build_days;
} BOOT_FirmwareId_t;

typedef struct
{
  uint32_t            magic;
  uint32_t            device_type;
  BOOT_FirmwareId_t   firmware_id;
  uint32_t            payload_size;
  uint32_t            payload_crc32;
  uint32_t            reserved;
  uint32_t            header_crc32;
} BOOT_FW_Header_t;

#define BOOT_SETTINGS_FIXED_SIZE 44U

typedef struct
{
  uint32_t            magic;
  uint32_t            sequence;
  BOOT_FirmwareId_t   active_id;
  uint32_t            active_crc32;
  uint32_t            active_size;
  BOOT_FirmwareId_t   backup_id;
  uint32_t            backup_crc32;
  uint32_t            backup_size;
  uint32_t            data_crc32;
  uint8_t             data[BOOT_SETTINGS_SIZE - BOOT_SETTINGS_FIXED_SIZE];
} BOOT_Settings_t;

_Static_assert(sizeof(BOOT_FirmwareId_t) == 8U, "BOOT_FirmwareId_t size");
_Static_assert(sizeof(BOOT_FW_Header_t) == 32U, "BOOT_FW_Header_t size");
_Static_assert(sizeof(BOOT_Settings_t) == BOOT_SETTINGS_SIZE, "BOOT_Settings_t must fit one flash page");
_Static_assert((BOOT_MAIN_APP_ADDR + BOOT_MAIN_APP_SIZE) == BOOT_BACKUP_APP_ADDR, "Main/backup overlap");
_Static_assert((BOOT_BACKUP_APP_ADDR + BOOT_BACKUP_APP_SIZE) == BOOT_SETTINGS1_ADDR, "Backup/settings overlap");
_Static_assert((BOOT_SETTINGS2_ADDR + BOOT_SETTINGS_SIZE) == BOOT_FLASH_END, "Settings must end at flash end");
_Static_assert((BOOT_SETTINGS1_ADDR % BOOT_FLASH_PAGE_SIZE) == 0U, "Settings page alignment");
_Static_assert((BOOT_SETTINGS2_ADDR % BOOT_FLASH_PAGE_SIZE) == 0U, "Settings page alignment");

typedef enum
{
  BOOT_MSC_STATE_IDLE = 0,
  BOOT_MSC_STATE_WAIT_HEADER,
  BOOT_MSC_STATE_RECEIVING,
  BOOT_MSC_STATE_REJECTED
} BOOT_MSC_State_t;

#endif
