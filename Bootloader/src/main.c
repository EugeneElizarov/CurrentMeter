#include "stm32f3xx.h"
#include "bsp_rcc.h"
#include "bsp_led.h"
#include "bsp_usb_msc.h"
#include "bsp_flash.h"
#include "bsp_settings.h"
#include "boot_crc32.h"
#include "bootloader_defs.h"

static bool vector_valid(uint32_t addr, uint32_t size)
{
    uint32_t sp=*(volatile uint32_t *)addr;
    uint32_t reset=*(volatile uint32_t *)(addr+4U);
    if ((sp & 0x2FFE0000U)!=0x20000000U) return false;
    if ((reset & 1U)==0U || reset<addr || reset>=addr+size) return false;
    for (uint32_t i=2U;i<16U;i++) {
        uint32_t v=*(volatile uint32_t *)(addr+i*4U);
        if (v!=0U && ((v&1U)==0U || v<addr || v>=addr+size)) return false;
    }
    return true;
}

static bool copy_image(uint32_t src,uint32_t dst,uint32_t size)
{
    if (size==0U || size>BOOT_MAIN_APP_SIZE || (size&1U)) return false;
    if (!BSP_FLASH_Unlock()) return false;
    for (uint32_t a=dst;a<dst+BOOT_MAIN_APP_SIZE;a+=BOOT_FLASH_PAGE_SIZE)
        if (!BSP_FLASH_ErasePage(a)) { BSP_FLASH_Lock(); return false; }
    for (uint32_t a=0;a<size;a+=2U)
        if (!BSP_FLASH_WriteHalfWord(dst+a,*(volatile uint16_t *)(src+a))) { BSP_FLASH_Lock(); return false; }
    BSP_FLASH_Lock();
    return BOOT_CalcCRC32(dst,size)==BOOT_CalcCRC32(src,size) && vector_valid(dst,size);
}

static void jump_to_app(uint32_t addr,uint32_t size)
{
    if (!vector_valid(addr,size)) return;
    __disable_irq();
    SysTick->CTRL=0; SysTick->LOAD=0; SysTick->VAL=0;
    SCB->VTOR=addr;
    __set_MSP(*(volatile uint32_t *)addr);
    ((void (*)(void))(*(volatile uint32_t *)(addr+4U)))();
}

int main(void)
{
    BSP_RCC_Init();
    BSP_LED_Init();
    const BOOT_Settings_t *s=BSP_SETTINGS_GetActive();

    if (s) {
        bool main_ok=vector_valid(BOOT_MAIN_APP_ADDR,s->active_size) &&
                     BOOT_CalcCRC32(BOOT_MAIN_APP_ADDR,s->active_size)==s->active_crc32;
        if (main_ok) jump_to_app(BOOT_MAIN_APP_ADDR,s->active_size);

        bool backup_ok=vector_valid(BOOT_BACKUP_APP_ADDR,s->backup_size) &&
                       BOOT_CalcCRC32(BOOT_BACKUP_APP_ADDR,s->backup_size)==s->backup_crc32;
        if (backup_ok && copy_image(BOOT_BACKUP_APP_ADDR,BOOT_MAIN_APP_ADDR,s->backup_size))
            jump_to_app(BOOT_MAIN_APP_ADDR,s->backup_size);
    }

    BSP_LED_Control(BSP_LED_FLASH_125MS);
    BSP_USB_MSC_Init();
    while (1) BSP_USB_MSC_Task();
}
void HardFault_Handler(void){while(1);}
void MemManage_Handler(void){while(1);}
void BusFault_Handler(void){while(1);}
void UsageFault_Handler(void){while(1);}
void SVC_Handler(void){}
void DebugMon_Handler(void){}
void PendSV_Handler(void){}
