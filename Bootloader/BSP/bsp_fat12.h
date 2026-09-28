#ifndef BSP_FAT12_H
#define BSP_FAT12_H

#include <stdint.h>
#include "bootloader_defs.h"

#define FAT12_SECTOR_SIZE       512U
#define FAT12_TOTAL_SECTORS     128U
#define FAT12_ROOT_LBA          5U
#define FAT12_DATA_START_LBA    6U
#define FAT12_UPDATE_SECTORS    ((FAT12_UPDATE_SIZE + FAT12_SECTOR_SIZE - 1U) / FAT12_SECTOR_SIZE)
#define FAT12_UPDATE_END_LBA    (FAT12_DATA_START_LBA + FAT12_UPDATE_SECTORS)

void BSP_FAT12_Init(void);
const uint8_t *BSP_FAT12_GetSectorPtr(uint32_t lba);

#endif
