// File: bsp_fat12.c
#include "bsp_fat12.h"
#include <string.h>

/* Структура загрузочного сектора FAT12 */
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

/* Структура записи корневого каталога */
typedef struct __attribute__((packed))
{
    uint8_t  name[11];
    uint8_t  attr;
    uint8_t  reserved[10];
    uint16_t first_cluster;
    uint32_t file_size;
} FAT12_DirEntry_t;

static uint8_t fat12_disk[FAT12_TOTAL_SECTORS * FAT12_SECTOR_SIZE] __attribute__((aligned(4)));

void BSP_FAT12_Init(void)
{
    memset(fat12_disk, 0, sizeof(fat12_disk));

    /* 1. Заполняем Boot Sector (LBA 0) */
    FAT12_BootSector_t *boot = (FAT12_BootSector_t *)fat12_disk;
    boot->jump_boot[0] = 0xEB; boot->jump_boot[1] = 0x3C; boot->jump_boot[2] = 0x90;
    memcpy(boot->oem_name, "MSDOS5.0", 8);
    boot->bytes_per_sector = 512;
    boot->sectors_per_cluster = 1;
    boot->reserved_sector_count = 1;
    boot->num_fats = 2;
    boot->root_entry_count = 16; /* 1 сектор */
    boot->total_sectors_16 = FAT12_TOTAL_SECTORS;
    boot->media_type = 0xF8;
    boot->fat_size_16 = 2; /* 2 сектора на FAT */
    boot->sectors_per_track = 1;
    boot->number_of_heads = 1;
    boot->total_sectors_32 = FAT12_TOTAL_SECTORS;
    boot->drive_number = 0x80;
    boot->boot_signature = 0x29;
    boot->volume_id = 0x12345678;
    memcpy(boot->volume_label, "BRACELET   ", 11);
    memcpy(boot->fs_type, "FAT12   ", 8);
    boot->boot_sector_sign = 0xAA55;

    /* 2. Заполняем FAT1 и FAT2 (LBA 1 и LBA 2) */
    /* Media byte + EOF для кластера 0 и 1 */
    uint8_t *fat = &fat12_disk[1 * FAT12_SECTOR_SIZE];
    fat[0] = 0xF8; fat[1] = 0xFF; fat[2] = 0xFF; /* Кластер 0 и 1 зарезервированы */
    /* Кластер 2 (наш файл) -> EOF (0xFFF) */
    fat[3] = 0xFF; fat[4] = 0xFF; 
    
    /* Копируем FAT1 в FAT2 */
    memcpy(&fat12_disk[3 * FAT12_SECTOR_SIZE], fat, FAT12_SECTOR_SIZE);

    /* 3. Заполняем Root Directory (LBA 3) */
    FAT12_DirEntry_t *root = (FAT12_DirEntry_t *)&fat12_disk[3 * FAT12_SECTOR_SIZE];
    memcpy(root->name, "UPDATE  BIN", 11);
    root->attr = 0x20; /* Archive */
    root->first_cluster = 2;
    root->file_size = BOOT_BACKUP_APP_SIZE + 32; /* 48KB + 32 байта заголовка */
}

const uint8_t* BSP_FAT12_GetSectorPtr(uint32_t lba)
{
    if (lba < FAT12_TOTAL_SECTORS)
    {
        return &fat12_disk[lba * FAT12_SECTOR_SIZE];
    }
    return NULL;
}