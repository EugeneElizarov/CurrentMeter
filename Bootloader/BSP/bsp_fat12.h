#ifndef BSP_FAT12_H
#define BSP_FAT12_H
#include <stdint.h>
#include "bootloader_defs.h"
#define FAT12_SECTOR_SIZE 512U
#define FAT12_TOTAL_SECTORS 128U
#define FAT12_ROOT_LBA 5U
#define FAT12_DATA_START_LBA 6U
#define FAT12_UPDATE_SIZE (BOOT_BACKUP_APP_SIZE + sizeof(BOOT_FW_Header_t))
void BSP_FAT12_Init(void);
const uint8_t *BSP_FAT12_GetSectorPtr(uint32_t lba);
#endif
