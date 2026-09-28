#include "bsp_fat12.h"
#include "bootloader_defs.h"
#include <string.h>

typedef struct __attribute__((packed))
{
    uint8_t  jump_boot[3];
    uint8_t  oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t  num_fats;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t number_of_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    uint8_t  volume_label[11];
    uint8_t  fs_type[8];
    uint8_t  boot_code[448];
    uint16_t boot_sector_sign;
} FAT12_BootSector_t;

typedef struct __attribute__((packed))
{
    uint8_t  name[11];
    uint8_t  attr;
    uint8_t  reserved[10];
    uint16_t first_cluster;
    uint32_t file_size;
} FAT12_DirEntry_t;

_Static_assert(sizeof(FAT12_BootSector_t) == FAT12_SECTOR_SIZE, "FAT12 boot sector size");
_Static_assert(sizeof(FAT12_DirEntry_t) == 32U, "FAT12 directory entry size");

static uint8_t boot_sector[FAT12_SECTOR_SIZE] __attribute__((aligned(4)));
static uint8_t fat[FAT12_SECTOR_SIZE * 2U] __attribute__((aligned(4)));
static uint8_t root_dir[FAT12_SECTOR_SIZE] __attribute__((aligned(4)));
static uint8_t empty_sector[FAT12_SECTOR_SIZE] __attribute__((aligned(4)));

static void fat12_set(uint8_t *fat_data, uint16_t cluster, uint16_t value)
{
    uint32_t off = cluster + cluster / 2U;

    if (cluster & 1U)
    {
        fat_data[off] = (uint8_t)((fat_data[off] & 0x0FU) | ((value & 0x0FFFU) << 4));
        fat_data[off + 1U] = (uint8_t)(value >> 4);
    }
    else
    {
        fat_data[off] = (uint8_t)value;
        fat_data[off + 1U] = (uint8_t)((fat_data[off + 1U] & 0xF0U) | (value >> 8));
    }
}

void BSP_FAT12_Init(void)
{
    memset(boot_sector, 0, sizeof(boot_sector));
    memset(fat, 0, sizeof(fat));
    memset(root_dir, 0, sizeof(root_dir));
    memset(empty_sector, 0, sizeof(empty_sector));

    FAT12_BootSector_t *b = (FAT12_BootSector_t *)boot_sector;

    b->jump_boot[0] = 0xEB;
    b->jump_boot[1] = 0x3C;
    b->jump_boot[2] = 0x90;
    memcpy(b->oem_name, "MSDOS5.0", 8U);
    b->bytes_per_sector = FAT12_SECTOR_SIZE;
    b->sectors_per_cluster = 1U;
    b->reserved_sector_count = 1U;
    b->num_fats = 2U;
    b->root_entry_count = 16U;
    b->total_sectors_16 = FAT12_TOTAL_SECTORS;
    b->media_type = 0xF8U;
    b->fat_size_16 = 2U;
    b->sectors_per_track = 1U;
    b->number_of_heads = 1U;
    b->boot_signature = 0x29U;
    b->volume_id = 0x43524D31U;
    memcpy(b->volume_label, "CURRENTMETER", 11U);
    memcpy(b->fs_type, "FAT12   ", 8U);
    b->boot_sector_sign = 0xAA55U;

    uint8_t *fat1 = &fat[0];
    uint8_t *fat2 = &fat[FAT12_SECTOR_SIZE];

    fat1[0] = 0xF8U;
    fat1[1] = 0xFFU;
    fat1[2] = 0xFFU;

    const uint16_t sectors =
        (uint16_t)((FAT12_UPDATE_SIZE + FAT12_SECTOR_SIZE - 1U) / FAT12_SECTOR_SIZE);

    for (uint16_t c = 0U; c < sectors; ++c)
    {
        const uint16_t cluster = (uint16_t)(2U + c);
        const uint16_t next = (c + 1U < sectors) ? (uint16_t)(cluster + 1U) : 0x0FFFU;
        fat12_set(fat1, cluster, next);
    }

    memcpy(fat2, fat1, FAT12_SECTOR_SIZE);

    FAT12_DirEntry_t *root = (FAT12_DirEntry_t *)root_dir;
    memcpy(root->name, "UPDATE  BIN", 11U);
    root->attr = 0x20U;
    root->first_cluster = 2U;
    root->file_size = FAT12_UPDATE_SIZE;
}

const uint8_t *BSP_FAT12_GetSectorPtr(uint32_t lba)
{
    if (lba >= FAT12_TOTAL_SECTORS)
        return 0;

    if (lba == 0U)
        return boot_sector;

    if (lba >= 1U && lba <= 4U)
        return &fat[(lba - 1U) * FAT12_SECTOR_SIZE];

    if (lba == FAT12_ROOT_LBA)
        return root_dir;

    return empty_sector;
}
