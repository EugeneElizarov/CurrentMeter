// File: bsp_fat12.h
#ifndef BSP_FAT12_H
#define BSP_FAT12_H

#include <stdint.h>

#define FAT12_SECTOR_SIZE       512U
#define FAT12_TOTAL_SECTORS     128U  /* 64 KB virtual disk */
#define FAT12_DATA_START_LBA    4U

void BSP_FAT12_Init(void);
const uint8_t* BSP_FAT12_GetSectorPtr(uint32_t lba);

#endif /* BSP_FAT12_H */