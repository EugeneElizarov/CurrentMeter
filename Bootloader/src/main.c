// File: main.c
#include "stm32f303xb.h"
#include "bsp_rcc.h"
#include "bsp_led.h"
#include "bsp_usb_msc.h"
#include "boot_crc32.h"
#include "bootloader_defs.h"

void SystemInit(void)
{
    /* Базовая инициализация FPU и тактирования от CMSIS */
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));
#endif
}

static void BOOT_JumpToApp(uint32_t app_addr)
{
    if (((*(volatile uint32_t *)app_addr) & 0x2FFE0000U) != 0x20000000U) return;

    __disable_irq();
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    uint32_t app_stack = *(volatile uint32_t *)app_addr;
    uint32_t app_reset = *(volatile uint32_t *)(app_addr + 4U);

    __set_MSP(app_stack);
    SCB->VTOR = app_addr;

    void (*app_entry)(void) = (void (*)(void))app_reset;
    app_entry();
}

int main(void)
{
    BSP_RCC_Init();
    BSP_LED_Init();
    BSP_LED_Control(BSP_LED_FLASH_500MS);

    BOOT_Settings_t *settings = (BOOT_Settings_t *)BOOT_SETTINGS_ADDR;

    if (settings->magic != BOOT_SETTINGS_MAGIC)
    {
        BSP_LED_Control(BSP_LED_ON); /* Индикация первого запуска */
        NVIC_SystemReset();
    }

    uint32_t main_crc = BOOT_CalcCRC32(BOOT_MAIN_APP_ADDR, BOOT_MAIN_APP_SIZE);
    bool main_valid = (main_crc == settings->main_crc);

    if (main_valid)
    {
        if (settings->main_id == settings->backup_id)
        {
            BSP_LED_Control(BSP_LED_OFF);
            BOOT_JumpToApp(BOOT_MAIN_APP_ADDR);
        }
        else
        {
            uint32_t backup_crc = BOOT_CalcCRC32(BOOT_BACKUP_APP_ADDR, BOOT_BACKUP_APP_SIZE);
            BSP_LED_Control(BSP_LED_FLASH_250MS);

            if (backup_crc == settings->backup_crc)
            {
                /* Копируем Backup в Main */
                /* (Логика копирования опущена для краткости, аналогична записи Flash) */
                settings->main_crc = backup_crc;
                settings->main_id = settings->backup_id;
            }
            else
            {
                settings->backup_crc = main_crc;
                settings->backup_id = settings->main_id;
            }
            BOOT_JumpToApp(BOOT_MAIN_APP_ADDR);
        }
    }
    else
    {
        uint32_t backup_crc = BOOT_CalcCRC32(BOOT_BACKUP_APP_ADDR, BOOT_BACKUP_APP_SIZE);
        if (backup_crc == settings->backup_crc)
        {
            BSP_LED_Control(BSP_LED_FLASH_250MS);
            /* Копируем Backup в Main */
            settings->main_crc = backup_crc;
            BOOT_JumpToApp(BOOT_MAIN_APP_ADDR);
        }
        else
        {
            /* Обе прошивки битые. Запускаем USB MSC */
            BSP_LED_Control(BSP_LED_FLASH_125MS);
            BSP_USB_MSC_Init();
            while (1) BSP_USB_MSC_Task();
        }
    }

    while (1);
}

void HardFault_Handler(void) { while (1); }
void MemManage_Handler(void) { while (1); }
void BusFault_Handler(void) { while (1); }
void UsageFault_Handler(void) { while (1); }
void SVC_Handler(void) {}
void DebugMon_Handler(void) {}
void PendSV_Handler(void) {}