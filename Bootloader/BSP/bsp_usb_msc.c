// File: bsp_usb_msc.c
#include "bsp_usb_msc.h"
#include "bsp_fat12.h"
#include "bsp_flash.h"
#include "bsp_aes.h"
#include "boot_crc32.h"
#include "bootloader_defs.h"
#include "bootloader_config.h"
#include "tusb.h"
#include <string.h>

static BOOT_MSC_State_t msc_state = BOOT_MSC_STATE_IDLE;
static BOOT_FW_Header_t fw_header;
static uint32_t bytes_written = 0;
static BSP_AES_Context_t aes_ctx;

void BSP_USB_MSC_Init(void)
{
    BSP_FAT12_Init();
    
    const uint8_t aes_key[BSP_AES_KEY_SIZE] = BOOT_AES_KEY;
    BSP_AES_Init(&aes_ctx, aes_key);

    tusb_init();
    msc_state = BOOT_MSC_STATE_WAIT_HEADER;
    bytes_written = 0;
}

void BSP_USB_MSC_Task(void)
{
    tud_task();
}

void tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize)
{
    const uint8_t *sector = BSP_FAT12_GetSectorPtr(lba);
    if (sector)
    {
        memcpy(buffer, sector + offset, bufsize);
    }
    else
    {
        memset(buffer, 0, bufsize);
    }
}

void tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize)
{
    if (msc_state == BOOT_MSC_STATE_REJECTED) return;

    /* Нас интересуют только записи в область данных (LBA >= 4) */
    if (lba < FAT12_DATA_START_LBA) return;

    uint32_t flash_addr = BOOT_BACKUP_APP_ADDR + bytes_written;

    /* Если это самый первый блок данных, он содержит заголовок */
    if (msc_state == BOOT_MSC_STATE_WAIT_HEADER)
    {
        memcpy(&fw_header, buffer, sizeof(BOOT_FW_Header_t));

        bool is_valid = true;
        if (fw_header.magic != BOOT_FW_MAGIC) is_valid = false;
        if (fw_header.device_type != DEVICE_TYPE_BRACELET) is_valid = false;
        if (fw_header.payload_size > BOOT_BACKUP_APP_SIZE) is_valid = false;
        if (BOOT_CalcCRC32((uint32_t)&fw_header, 20) != fw_header.header_crc32) is_valid = false;

        if (!is_valid)
        {
            msc_state = BOOT_MSC_STATE_REJECTED;
            tud_msc_set_sense(lun, SCSI_SENSE_DATA_PROTECT, 0x27, 0x00);
            return;
        }

        /* Заголовок валиден. Стираем Backup блок и начинаем писать */
        BSP_FLASH_Unlock();
        for (uint32_t page = BOOT_BACKUP_APP_ADDR; page < BOOT_BACKUP_APP_ADDR + BOOT_BACKUP_APP_SIZE; page += BOOT_FLASH_PAGE_SIZE)
        {
            BSP_FLASH_ErasePage(page);
        }

        msc_state = BOOT_MSC_STATE_RECEIVING;
        
        /* Пропускаем 32 байта заголовка в первом блоке */
        buffer += sizeof(BOOT_FW_Header_t);
        bufsize -= sizeof(BOOT_FW_Header_t);
        bytes_written += sizeof(BOOT_FW_Header_t);
    }

    /* Расшифровываем и записываем во Flash */
    uint8_t decrypted_block[BSP_AES_BLOCK_SIZE];
    uint32_t blocks = bufsize / BSP_AES_BLOCK_SIZE;

    for (uint32_t i = 0; i < blocks; i++)
    {
        BSP_AES_DecryptBlock(&aes_ctx, buffer + (i * BSP_AES_BLOCK_SIZE), decrypted_block);
        
        for (uint8_t w = 0; w < 4; w++)
        {
            uint32_t word = *(uint32_t *)&decrypted_block[w * 4];
            BSP_FLASH_WriteWord(flash_addr + (i * BSP_AES_BLOCK_SIZE) + (w * 4), word);
        }
    }

    bytes_written += blocks * BSP_AES_BLOCK_SIZE;

    if (bytes_written >= fw_header.payload_size)
    {
        BSP_FLASH_Lock();
        NVIC_SystemReset();
    }
}

bool tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize)
{
    void const *response = NULL;
    uint16_t resplen = 0;

    switch (scsi_cmd[0])
    {
        case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
            resplen = 0;
            break;

        default:
            tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
            return false;
    }

    if (response && (resplen > 0))
    {
        if (resplen > bufsize) resplen = bufsize;
        memcpy(buffer, response, resplen);
    }
    return true;
}

bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
    if (msc_state == BOOT_MSC_STATE_REJECTED)
    {
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3A, 0x00);
        return false;
    }
    return true;
}

void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
    (void) lun;
    memcpy(vendor_id, "STMicro", 8);
    memcpy(product_id, "Bracelet Boot", 16);
    memcpy(product_rev, "1.00", 4);
}

bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
{
    (void) lun; (void) power_condition; (void) start; (void) load_eject;
    return true;
}

int32_t tud_msc_read_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size)
{
    (void) lun;
    *block_count = FAT12_TOTAL_SECTORS;
    *block_size = FAT12_SECTOR_SIZE;
    return 0;
}