#include "stm32f3xx.h"
#include "bsp_rcc.h"
#include "bsp_led.h"
#include "bsp_usb_msc.h"
#include "bsp_flash.h"
#include "bsp_settings.h"
#include "boot_crc32.h"
#include "bootloader_defs.h"

static bool vector_valid(uint32_t addr,uint32_t size)
{
    if(size<64U||addr<BOOT_FLASH_BASE||size>BOOT_FLASH_SIZE-(addr-BOOT_FLASH_BASE))return false;
    {
        const uint32_t sp=*(volatile uint32_t *)(uintptr_t)addr;
        const uint32_t reset=*(volatile uint32_t *)(uintptr_t)(addr+4U);
        if((sp&7U)!=0U||sp<SRAM_BASE||sp>SRAM_BASE+32U*1024U)return false;
        if((reset&1U)==0U||reset<addr||reset>=addr+size)return false;
    }
    {
        uint32_t i;
        for(i=2U;i<16U;++i){
            const uint32_t v=*(volatile uint32_t *)(uintptr_t)(addr+i*4U);
            if(v!=0U&&((v&1U)==0U||v<addr||v>=addr+size))return false;
        }
    }
    return true;
}

static bool copy_image(uint32_t src,uint32_t dst,uint32_t size)
{
    uint32_t a;
    if(size==0U||size>BOOT_MAIN_APP_SIZE||(size&1U)!=0U||!BSP_FLASH_Unlock())return false;
    for(a=dst;a<dst+BOOT_MAIN_APP_SIZE;a+=BOOT_FLASH_PAGE_SIZE){
        if(!BSP_FLASH_ErasePage(a)){BSP_FLASH_Lock();return false;}
    }
    for(a=0U;a<size;a+=2U){
        if(!BSP_FLASH_WriteHalfWord(dst+a,*(volatile uint16_t *)(uintptr_t)(src+a))){BSP_FLASH_Lock();return false;}
    }
    BSP_FLASH_Lock();
    return BOOT_CalcCRC32(dst,size)==BOOT_CalcCRC32(src,size)&&vector_valid(dst,size);
}

static void jump_to_app(uint32_t addr,uint32_t size)
{
    if(!vector_valid(addr,size))return;
    __disable_irq();
    SysTick->CTRL=0U;SysTick->LOAD=0U;SysTick->VAL=0U;
    SCB->VTOR=addr;__DSB();__ISB();__set_MSP(*(volatile uint32_t *)(uintptr_t)addr);
    ((void (*)(void))(uintptr_t)(*(volatile uint32_t *)(uintptr_t)(addr+4U)))();
}

int main(void)
{
    BSP_RCC_Init();BSP_LED_Init();
    {
        const BOOT_Settings_t *s=BSP_SETTINGS_GetActive();
        if(s!=0){
            const bool main_ok=vector_valid(BOOT_MAIN_APP_ADDR,s->active_size)&&BOOT_CalcCRC32(BOOT_MAIN_APP_ADDR,s->active_size)==s->active_crc32;
            if(main_ok)jump_to_app(BOOT_MAIN_APP_ADDR,s->active_size);
            {
                const bool backup_ok=vector_valid(BOOT_BACKUP_APP_ADDR,s->backup_size)&&BOOT_CalcCRC32(BOOT_BACKUP_APP_ADDR,s->backup_size)==s->backup_crc32;
                if(backup_ok&&copy_image(BOOT_BACKUP_APP_ADDR,BOOT_MAIN_APP_ADDR,s->backup_size))jump_to_app(BOOT_MAIN_APP_ADDR,s->backup_size);
            }
        }
    }
    BSP_LED_Control(BSP_LED_FLASH_125MS);BSP_USB_MSC_Init();
    while(1)BSP_USB_MSC_Task();
}

void USB_LP_CAN_RX0_IRQHandler(void){USB_Device_IRQHandler();}
void HardFault_Handler(void){while(1){}}
void MemManage_Handler(void){while(1){}}
void BusFault_Handler(void){while(1){}}
void UsageFault_Handler(void){while(1){}}
void SVC_Handler(void){}
void DebugMon_Handler(void){}
void PendSV_Handler(void){}
