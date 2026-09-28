#include "usb_msc.h"
#include "usb_fsdev.h"
#include "bsp_fat12.h"
#include "bsp_flash.h"
#include "bsp_aes.h"
#include "bsp_settings.h"
#include "boot_crc32.h"
#include "bootloader_defs.h"
#include "bootloader_config.h"
#include "stm32f3xx.h"
#include <string.h>

#define CBW_SIG 0x43425355UL
#define CSW_SIG 0x53425355UL
#define STATE_IDLE 0U
#define STATE_IN 1U
#define STATE_OUT 2U
#define STATE_CSW 3U
#define SCSI_TUR 0x00U
#define SCSI_SENSE 0x03U
#define SCSI_INQUIRY 0x12U
#define SCSI_MODE_SENSE 0x1AU
#define SCSI_START_STOP 0x1BU
#define SCSI_PREVENT 0x1EU
#define SCSI_CAPACITY 0x25U
#define SCSI_READ10 0x28U
#define SCSI_WRITE10 0x2AU
#define REQ_GET_LUN 0xFEU
#define REQ_RESET 0xFFU

static uint8_t cbw[31], csw[13], packet[64], sector[512], sense[18], cdb[16];
static uint16_t cbw_n, sector_n;
static uint32_t tag, xfer_len, xfer_done, read_lba, read_len, write_lba;
static uint8_t state, sense_key, sense_asc, sense_ascq;
static BOOT_FW_Header_t header;
static uint32_t payload_written;
static bool update_active, update_rejected;
static BSP_AES_Context_t aes;
static uint8_t cbc_prev[16];

static uint16_t be16(const uint8_t *p)
{
  return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static uint32_t be32(const uint8_t *p)
{
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
         ((uint32_t)p[2] << 8) | p[3];
}

static void wb32(uint8_t *p, uint32_t v)
{
  p[0] = (uint8_t)(v >> 24);
  p[1] = (uint8_t)(v >> 16);
  p[2] = (uint8_t)(v >> 8);
  p[3] = (uint8_t)v;
}

static void set_sense(uint8_t key, uint8_t asc, uint8_t ascq)
{
  sense_key = key;
  sense_asc = asc;
  sense_ascq = ascq;
}

static void send_csw(uint8_t status)
{
  uint32_t residue = xfer_len > xfer_done ? xfer_len - xfer_done : 0U;
  wb32(csw, CSW_SIG);
  wb32(csw + 4U, tag);
  wb32(csw + 8U, residue);
  csw[12] = status;
  state = STATE_CSW;
  USB_EP_Send(1U, csw, 13U);
}

static void send_in(const uint8_t *data, uint16_t length)
{
  if (length > xfer_len - xfer_done)
    length = (uint16_t)(xfer_len - xfer_done);
  memcpy(packet, data, length);
  xfer_done += length;
  state = STATE_IN;
  USB_EP_Send(1U, packet, length);
}

static void cbc_reset(void)
{
  const uint8_t iv[16] = BOOT_AES_IV;
  memcpy(cbc_prev, iv, sizeof(cbc_prev));
}

static bool decrypt_block(const uint8_t *cipher, uint32_t dst)
{
  uint8_t dec[16];
  uint8_t plain[16];
  uint16_t i;

  BSP_AES_DecryptBlock(&aes, cipher, dec);
  for(i = 0U; i < 16U; ++i)
    plain[i] = (uint8_t)(dec[i] ^ cbc_prev[i]);

  memcpy(cbc_prev, cipher, 16U);

  for(i = 0U; i < 16U; i += 2U)
  {
    uint16_t halfword = (uint16_t)plain[i] | ((uint16_t)plain[i + 1U] << 8);

    if (!BSP_FLASH_WriteHalfWord(dst + i, halfword)) 
      return false;
  }

  return true;
}

static bool valid_header(void)
{
  uint32_t crc = header.header_crc32;
  BOOT_FW_Header_t check = header;

  check.header_crc32 = 0U;

  return check.magic == BOOT_FW_MAGIC &&
         check.device_type == DEVICE_TYPE_CURRENTMETER &&
         check.payload_size > 0U &&
         check.payload_size <= BOOT_BACKUP_APP_SIZE &&
        (check.payload_size % 16U) == 0U &&
         check.firmware_id.cpu_id == (uint16_t)(DBGMCU->IDCODE & 0x0FFFU) &&
         BOOT_CalcCRC32((uint32_t)(uintptr_t)&check, sizeof(check) - sizeof(check.header_crc32)) == crc;
}

static bool erase_backup(void)
{
  uint32_t address;

  if(!BSP_FLASH_Unlock())
    return false;

  for(address = BOOT_BACKUP_APP_ADDR;
      address < BOOT_BACKUP_APP_ADDR + BOOT_BACKUP_APP_SIZE;
      address += BOOT_FLASH_PAGE_SIZE)
  {
    if (!BSP_FLASH_ErasePage(address))
    {
      BSP_FLASH_Lock();
      return false;
    }
  }

  return true;
}

static bool finish_update(void)
{
  BOOT_Settings_t settings;
  const BOOT_Settings_t *old;

  BSP_FLASH_Lock();

  if (BOOT_CalcCRC32(BOOT_BACKUP_APP_ADDR, header.payload_size) != header.payload_crc32)
    return false;

  if (!(*(volatile uint32_t *)(uintptr_t)BOOT_BACKUP_APP_ADDR) ||
      ((*(volatile uint32_t *)(uintptr_t)(BOOT_BACKUP_APP_ADDR + 4U) & 1U) == 0U))
    return false;

  old = BSP_SETTINGS_GetActive();
  memset(&settings, 0, sizeof(settings));

  if (old != 0)
    memcpy(&settings, old, sizeof(settings));

  settings.magic = BOOT_SETTINGS_MAGIC;
  settings.sequence = old != 0 ? old->sequence + 1U : 1U;
  settings.active_id = header.firmware_id;
  settings.active_crc32 = header.payload_crc32;
  settings.active_size = header.payload_size;
  settings.backup_id = header.firmware_id;
  settings.backup_crc32 = header.payload_crc32;
  settings.backup_size = header.payload_size;

  if (!BSP_SETTINGS_Store(&settings))
    return false;

  NVIC_SystemReset();
  return true;
}

static bool process_sector(uint32_t lba, const uint8_t *data)
{
  uint32_t offset = 0U;
  uint32_t remaining;
  uint16_t count;
  uint16_t i;

  if(!update_active)
  {
    const uint8_t key[BSP_AES_KEY_SIZE] = BOOT_AES_KEY;

    if (lba != FAT12_DATA_START_LBA)
      return true;

    memcpy(&header, data, sizeof(header));

    if (!valid_header())
    {
      update_rejected = true;
      set_sense(0x07U, 0x27U, 0U);
      return false;
    }

    if (!erase_backup())
    {
      update_rejected = true;
      set_sense(0x04U, 0x44U, 0U);
      return false;
    }

    BSP_AES_Init(&aes, key);
    cbc_reset();
    payload_written = 0U;
    update_active = true;
    offset = sizeof(header);
  }

  if (lba != FAT12_DATA_START_LBA +
     (payload_written + sizeof(header)) / FAT12_SECTOR_SIZE)
    return false;

  remaining = header.payload_size - payload_written;
  count = (uint16_t)(remaining > FAT12_SECTOR_SIZE - offset ?
          FAT12_SECTOR_SIZE - offset : remaining);

  if ((count % 16U) != 0U)
    return false;

  for(i = 0U; i < count; i += 16U)
  {
    if  (!decrypt_block(data + offset + i,
        BOOT_BACKUP_APP_ADDR + payload_written + i))
      return false;
  }

  payload_written += count;

  if (payload_written == header.payload_size)
    return finish_update();

  return true;
}

static void read_next(void)
{
  uint32_t lba;
  uint16_t offset;
  uint16_t count;
  const uint8_t *data;

  if (xfer_done >= xfer_len)
  {
    send_csw(0U);
    return;
  }

  lba = read_lba + xfer_done / FAT12_SECTOR_SIZE;
  offset = (uint16_t)(xfer_done % FAT12_SECTOR_SIZE);
  data = BSP_FAT12_GetSectorPtr(lba);

  if (data == 0)
  {
    set_sense(0x03U, 0x11U, 0U);
    send_csw(1U);
    return;
  }

  count = (uint16_t)(xfer_len - xfer_done);
  if(count > 64U)
    count = 64U;
  if(count > FAT12_SECTOR_SIZE - offset)
    count = (uint16_t)(FAT12_SECTOR_SIZE - offset);

  send_in(data + offset, count);
}

static void command_start(void)
{
  uint8_t opcode = cdb[0];
  uint8_t response[36];
  uint8_t capacity[8];
  uint8_t mode[4];
  uint16_t blocks;
  uint32_t lba;

  xfer_done = 0U;

  switch(opcode)
  {
    case SCSI_TUR:
    case SCSI_START_STOP:
    case SCSI_PREVENT:
    {
      send_csw(0U);
      break;
    }
    case SCSI_INQUIRY:
    {
      memset(response, 0, sizeof(response));
      response[1] = 0x80U;
      response[2] = 5U;
      response[3] = 2U;
      response[4] = 31U;
      memcpy(response + 8U, "CurrentM", 8U);
      memcpy(response + 16U, "CurrentMeter     ", 16U);
      memcpy(response + 32U, "1.00", 4U);
      send_in(response, 36U);
      break;
    }
    case SCSI_SENSE:
    {
      memset(sense, 0, sizeof(sense));
      sense[0] = 0x70U;
      sense[2] = sense_key;
      sense[7] = 10U;
      sense[12] = sense_asc;
      sense[13] = sense_ascq;
      send_in(sense, sizeof(sense));
      set_sense(0U, 0U, 0U);
      break;
    }
    case SCSI_MODE_SENSE:
    {
      mode[0] = 3U;
      mode[1] = 0U;
      mode[2] = 0U;
      mode[3] = 0U;
      send_in(mode, sizeof(mode));
      break;
    }
    case SCSI_CAPACITY:
    {
      wb32(capacity, FAT12_TOTAL_SECTORS - 1U);
      wb32(capacity + 4U, FAT12_SECTOR_SIZE);
      send_in(capacity, sizeof(capacity));
      break;
    }
    case SCSI_READ10:
    {
      lba = be32(cdb + 2U);
      blocks = be16(cdb + 7U);
      read_lba = lba;
      read_len = (uint32_t)blocks * FAT12_SECTOR_SIZE;

      if (read_len != xfer_len ||
          lba >= FAT12_TOTAL_SECTORS ||
          blocks > FAT12_TOTAL_SECTORS - lba)
      {
        set_sense(0x05U, 0x21U, 0U);
        send_csw(1U);
      }
      else
      {
        read_next();
      }
      break;
    }
    case SCSI_WRITE10:
    {
      lba = be32(cdb + 2U);
      blocks = be16(cdb + 7U);
      write_lba = lba;
      sector_n = 0U;

      if  (xfer_len != (uint32_t)blocks * FAT12_SECTOR_SIZE ||
          lba >= FAT12_TOTAL_SECTORS ||
          blocks > FAT12_TOTAL_SECTORS - lba)
      {
        set_sense(0x05U, 0x21U, 0U);
        send_csw(1U);
      }
      else
      {
        state = STATE_OUT;
        USB_EP_Receive(2U);
      }
      break;
    }
    default:
    {
        set_sense(0x05U, 0x20U, 0U);
        send_csw(1U);
        break;
    }
  }
}

void USB_MSC_Init(void)
{
  BSP_FAT12_Init();
  USB_MSC_Reset();
}

void USB_MSC_Reset(void)
{
  cbw_n = 0U;
  sector_n = 0U;
  state = STATE_IDLE;
  xfer_len = 0U;
  xfer_done = 0U;
  tag = 0U;
  payload_written = 0U;
  update_active = false;
  update_rejected = false;
  set_sense(0U, 0U, 0U);
}

void USB_MSC_Out(const uint8_t *data, uint16_t length)
{
  uint16_t i;

  if (state == STATE_OUT)
  {
    for(i = 0U; i < length; ++i)
    {
      sector[sector_n++] = data[i];
      xfer_done++;

      if (sector_n == FAT12_SECTOR_SIZE)
      {
        if (write_lba >= FAT12_DATA_START_LBA &&
            write_lba < FAT12_UPDATE_END_LBA &&
            !update_rejected &&
            !process_sector(write_lba, sector))
          update_rejected = true;

        sector_n = 0U;
        write_lba++;
      }
    }

    if (xfer_done >= xfer_len)
    {
      if (!update_rejected && sector_n != 0U)
        update_rejected = true;

      send_csw(update_rejected ? 1U : 0U);
    }

    return;
  }

  if (state != STATE_IDLE)
    return;

  if ((uint32_t)cbw_n + length > sizeof(cbw))
  {
    USB_EP_Stall(2U, false);
    return;
  }

  memcpy(&cbw[cbw_n], data, length);
  cbw_n = (uint16_t)(cbw_n + length);

  if (cbw_n < sizeof(cbw))
    return;

  cbw_n = 0U;

  if (be32(cbw) != CBW_SIG ||
      cbw[13] != 0U ||
      cbw[14] == 0U ||
      cbw[14] > 16U ||
     (cbw[12] & 0x7FU) != 0U)
  {
    USB_EP_Stall(2U, false);
    return;
  }

  tag = be32(cbw + 4U);
  xfer_len = be32(cbw + 8U);
  memcpy(cdb, cbw + 15U, sizeof(cdb));
  command_start();
}

void USB_MSC_InComplete(void)
{
  if (state == STATE_CSW)
  {
    state = STATE_IDLE;
    USB_EP_Receive(2U);
    return;
  }

  if (state == STATE_IN)
  {
    if (cdb[0] == SCSI_READ10)
      read_next();
    else 
    {
      if (xfer_done >= xfer_len)
        send_csw(0U);
    }
  }
}

bool USB_MSC_Control(const USB_SetupPacket_t *setup, const uint8_t **data, uint16_t *length)
{
  static uint8_t max_lun = 0U;

  if (setup->bmRequestType == 0xA1U &&
      setup->bRequest == REQ_GET_LUN &&
      setup->wLength == 1U)
  {
    *data = &max_lun;
    *length = 1U;
    return true;
  }

  if ((setup->bmRequestType & 0x7FU) == 0x21U &&
       setup->bRequest == REQ_RESET &&
       setup->wLength == 0U)
  {
    USB_MSC_Reset();
    *data = 0;
    *length = 0U;
    return true;
  }

  return false;
}
