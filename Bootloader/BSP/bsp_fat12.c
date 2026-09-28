#include "bsp_fat12.h"
#include "bootloader_defs.h"
#include <string.h>

typedef struct __attribute__((packed)) {
    uint8_t jump_boot[3]; uint8_t oem_name[8]; uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster; uint16_t reserved_sector_count; uint8_t num_fats;
    uint16_t root_entry_count; uint16_t total_sectors_16; uint8_t media_type;
    uint16_t fat_size_16; uint16_t sectors_per_track; uint16_t number_of_heads;
    uint32_t hidden_sectors; uint32_t total_sectors_32; uint8_t drive_number;
    uint8_t reserved1; uint8_t boot_signature; uint32_t volume_id;
    uint8_t volume_label[11]; uint8_t fs_type[8]; uint8_t boot_code[448];
    uint16_t boot_sector_sign;
} FAT12_BootSector_t;

typedef struct __attribute__((packed)) {
    uint8_t name[11]; uint8_t attr; uint8_t reserved[10];
    uint16_t first_cluster; uint32_t file_size;
} FAT12_DirEntry_t;

static uint8_t disk[FAT12_TOTAL_SECTORS * FAT12_SECTOR_SIZE] __attribute__((aligned(4)));

static void fat12_set(uint8_t *fat, uint16_t cluster, uint16_t value)
{
    uint32_t off = cluster + cluster / 2U;
    if (cluster & 1U) {
        fat[off] = (uint8_t)((fat[off] & 0x0FU) | ((value & 0x0FFFU) << 4));
        fat[off + 1U] = (uint8_t)(value >> 4);
    } else {
        fat[off] = (uint8_t)value;
        fat[off + 1U] = (uint8_t)((fat[off + 1U] & 0xF0U) | (value >> 8));
    }
}

void BSP_FAT12_Init(void)
{
    memset(disk, 0, sizeof(disk));
    FAT12_BootSector_t *b = (FAT12_BootSector_t *)disk;
    b->jump_boot[0]=0xEB; b->jump_boot[1]=0x3C; b->jump_boot[2]=0x90;
    memcpy(b->oem_name, "MSDOS5.0", 8); b->bytes_per_sector=512;
    b->sectors_per_cluster=1; b->reserved_sector_count=1; b->num_fats=2;
    b->root_entry_count=16; b->total_sectors_16=FAT12_TOTAL_SECTORS;
    b->media_type=0xF8; b->fat_size_16=2; b->sectors_per_track=1; b->number_of_heads=1;
    b->boot_signature=0x29; b->volume_id=0x43524D31U;
    memcpy(b->volume_label,"CURRENTMETER",11); memcpy(b->fs_type,"FAT12   ",8);
    b->boot_sector_sign=0xAA55;

    uint8_t *fat1=&disk[512]; uint8_t *fat2=&disk[1536];
    fat1[0]=0xF8; fat1[1]=0xFF; fat1[2]=0xFF;
    const uint16_t sectors=(uint16_t)((FAT12_UPDATE_SIZE + 511U)/512U);
    for (uint16_t c=0; c<sectors; ++c) fat12_set(fat1,(uint16_t)(2U+c),(c+1U<sectors)?(uint16_t)(3U+c):0xFFFU);
    memcpy(fat2,fat1,1024);

    FAT12_DirEntry_t *root=(FAT12_DirEntry_t *)&disk[FAT12_ROOT_LBA*512U];
    memcpy(root->name,"UPDATE  BIN",11); root->attr=0x20; root->first_cluster=2;
    root->file_size=FAT12_UPDATE_SIZE;
}

const uint8_t *BSP_FAT12_GetSectorPtr(uint32_t lba)
{
    return lba < FAT12_TOTAL_SECTORS ? &disk[lba * FAT12_SECTOR_SIZE] : 0;
}
