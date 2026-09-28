#include "bsp_usb_msc.h"
#include "bsp_fat12.h"
#include "bsp_flash.h"
#include "bsp_aes.h"
#include "bsp_settings.h"
#include "boot_crc32.h"
#include "bootloader_defs.h"
#include "bootloader_config.h"
#include "stm32f3xx.h"
#include "tusb.h"
#include <string.h>

static BOOT_MSC_State_t msc_state=BOOT_MSC_STATE_IDLE;
static BOOT_FW_Header_t fw_header;
static uint32_t payload_written;
static BSP_AES_Context_t aes_ctx;
static uint8_t cbc_prev[BSP_AES_BLOCK_SIZE];

void BSP_USB_MSC_Init(void)
{
    BSP_FAT12_Init();
    const uint8_t key[BSP_AES_KEY_SIZE]=BOOT_AES_KEY;
    BSP_AES_Init(&aes_ctx,key);
    memset(cbc_prev,0,sizeof(cbc_prev));
    tusb_init();
    msc_state=BOOT_MSC_STATE_WAIT_HEADER;
    payload_written=0U;
}

void BSP_USB_MSC_Task(void){ tud_task(); }

int32_t tud_msc_read10_cb(uint8_t lun,uint32_t lba,uint32_t offset,void *buffer,uint32_t bufsize)
{
    (void)lun;
    const uint8_t *sector=BSP_FAT12_GetSectorPtr(lba);
    if (!sector || offset+bufsize>FAT12_SECTOR_SIZE) return TUD_MSC_RET_ERROR;
    memcpy(buffer,sector+offset,bufsize);
    return (int32_t)bufsize;
}

int32_t tud_msc_write10_cb(uint8_t lun,uint32_t lba,uint32_t offset,uint8_t *buffer,uint32_t bufsize)
{
    if (msc_state==BOOT_MSC_STATE_REJECTED || lba<FAT12_DATA_START_LBA) return (int32_t)bufsize;
    if (offset!=0U) return TUD_MSC_RET_ERROR;

    if (msc_state==BOOT_MSC_STATE_WAIT_HEADER) {
        if (bufsize<sizeof(BOOT_FW_Header_t)) return TUD_MSC_RET_ERROR;
        memcpy(&fw_header,buffer,sizeof(fw_header));
        uint32_t hc=fw_header.header_crc32;
        fw_header.header_crc32=0U;
        bool ok=(fw_header.magic==BOOT_FW_MAGIC &&
                 fw_header.device_type==DEVICE_TYPE_CURRENTMETER &&
                 fw_header.payload_size>0U &&
                 fw_header.payload_size<=BOOT_BACKUP_APP_SIZE &&
                 (fw_header.payload_size%16U)==0U &&
                 fw_header.firmware_id.cpu_id==(uint16_t)(DBGMCU->IDCODE&0x0FFFU) &&
                 BOOT_CalcCRC32((uint32_t)&fw_header,28U)==hc);
        fw_header.header_crc32=hc;
        if (!ok) {
            msc_state=BOOT_MSC_STATE_REJECTED;
            tud_msc_set_sense(lun,SCSI_SENSE_DATA_PROTECT,0x27,0);
            return TUD_MSC_RET_ERROR;
        }

        if (!BSP_FLASH_Unlock()) return TUD_MSC_RET_ERROR;
        for (uint32_t a=BOOT_BACKUP_APP_ADDR;a<BOOT_BACKUP_APP_ADDR+BOOT_BACKUP_APP_SIZE;a+=BOOT_FLASH_PAGE_SIZE)
            if (!BSP_FLASH_ErasePage(a)) { BSP_FLASH_Lock(); return TUD_MSC_RET_ERROR; }
        /* Keep Flash unlocked for the complete image transfer. */
        payload_written=0U;
        memset(cbc_prev,0,sizeof(cbc_prev));
        msc_state=BOOT_MSC_STATE_RECEIVING;
        buffer+=sizeof(BOOT_FW_Header_t);
        bufsize-=sizeof(BOOT_FW_Header_t);
    }

    if (msc_state!=BOOT_MSC_STATE_RECEIVING) return (int32_t)bufsize;
    if ((bufsize%BSP_AES_BLOCK_SIZE)!=0U || payload_written+bufsize>fw_header.payload_size) return TUD_MSC_RET_ERROR;

    for (uint32_t off=0;off<bufsize;off+=BSP_AES_BLOCK_SIZE) {
        uint8_t dec[BSP_AES_BLOCK_SIZE],plain[BSP_AES_BLOCK_SIZE];
        BSP_AES_DecryptBlock(&aes_ctx,buffer+off,dec);
        for (uint32_t i=0;i<BSP_AES_BLOCK_SIZE;i++) plain[i]=(uint8_t)(dec[i]^cbc_prev[i]);
        memcpy(cbc_prev,buffer+off,BSP_AES_BLOCK_SIZE);
        for (uint32_t i=0;i<BSP_AES_BLOCK_SIZE;i+=2)
            if (!BSP_FLASH_WriteHalfWord(BOOT_BACKUP_APP_ADDR+payload_written+off+i,(uint16_t)plain[i]|((uint16_t)plain[i+1]<<8))) {
                BSP_FLASH_Lock();
                return TUD_MSC_RET_ERROR;
            }
    }
    payload_written+=bufsize;

    if (payload_written==fw_header.payload_size) {
        BSP_FLASH_Lock();
        if (BOOT_CalcCRC32(BOOT_BACKUP_APP_ADDR,fw_header.payload_size)!=fw_header.payload_crc32 ||
            !(*(volatile uint32_t *)BOOT_BACKUP_APP_ADDR) ||
            ((*(volatile uint32_t *)(BOOT_BACKUP_APP_ADDR+4U)&1U)==0U)) {
            msc_state=BOOT_MSC_STATE_REJECTED;
            tud_msc_set_sense(lun,SCSI_SENSE_DATA_PROTECT,0x10,0);
            return TUD_MSC_RET_ERROR;
        }

        BOOT_Settings_t s;
        const BOOT_Settings_t *old=BSP_SETTINGS_GetActive();
        memset(&s,0,sizeof(s));
        if (old) memcpy(&s,old,sizeof(s));
        s.magic=BOOT_SETTINGS_MAGIC;
        s.sequence=old?old->sequence+1U:1U;
        s.active_id=fw_header.firmware_id;
        s.active_crc32=fw_header.payload_crc32;
        s.active_size=fw_header.payload_size;
        s.backup_id=fw_header.firmware_id;
        s.backup_crc32=fw_header.payload_crc32;
        s.backup_size=fw_header.payload_size;
        if (!BSP_SETTINGS_Store(&s)) return TUD_MSC_RET_ERROR;
        NVIC_SystemReset();
    }
    return (int32_t)bufsize;
}

int32_t tud_msc_scsi_cb(uint8_t lun,uint8_t const scsi_cmd[16],void *buffer,uint16_t bufsize)
{
    (void)buffer;
    (void)bufsize;

    if (scsi_cmd[0]==SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL)
        return 0;

    tud_msc_set_sense(lun,SCSI_SENSE_ILLEGAL_REQUEST,0x20,0);
    return TUD_MSC_RET_ERROR;
}

bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
    if (msc_state==BOOT_MSC_STATE_REJECTED) { tud_msc_set_sense(lun,SCSI_SENSE_NOT_READY,0x3A,0); return false; }
    return true;
}
void tud_msc_inquiry_cb(uint8_t lun,uint8_t vendor_id[8],uint8_t product_id[16],uint8_t product_rev[4])
{
    (void)lun; memcpy(vendor_id,"CurrentM",8); memset(product_id,' ',16); memcpy(product_id,"CurrentMeter",12); memcpy(product_rev,"1.00",4);
}
bool tud_msc_start_stop_cb(uint8_t lun,uint8_t power_condition,bool start,bool load_eject)
{(void)lun;(void)power_condition;(void)start;(void)load_eject;return true;}
void tud_msc_capacity_cb(uint8_t lun,uint32_t *block_count,uint16_t *block_size)
{(void)lun;*block_count=FAT12_TOTAL_SECTORS;*block_size=FAT12_SECTOR_SIZE;}
