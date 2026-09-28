#include "bsp_settings.h"
#include "bsp_flash.h"
#include "boot_crc32.h"
#include <string.h>

static bool settings_crc_ok(const BOOT_Settings_t *s)
{
    BOOT_Settings_t tmp;
    memcpy(&tmp, s, sizeof(tmp));
    tmp.data_crc32 = 0U;
    return BOOT_CalcCRC32((uint32_t)&tmp, sizeof(tmp)) == s->data_crc32;
}

bool BSP_SETTINGS_Validate(const BOOT_Settings_t *settings)
{
    return settings && settings->magic == BOOT_SETTINGS_MAGIC && settings_crc_ok(settings);
}

const BOOT_Settings_t *BSP_SETTINGS_GetActive(void)
{
    const BOOT_Settings_t *a=(const BOOT_Settings_t *)BOOT_SETTINGS1_ADDR;
    const BOOT_Settings_t *b=(const BOOT_Settings_t *)BOOT_SETTINGS2_ADDR;
    if (BSP_SETTINGS_Validate(a)) return a;
    if (BSP_SETTINGS_Validate(b)) return b;
    return 0;
}

bool BSP_SETTINGS_Store(const BOOT_Settings_t *settings)
{
    BOOT_Settings_t tmp;
    memcpy(&tmp, settings, sizeof(tmp));
    tmp.data_crc32=0U;
    tmp.data_crc32=BOOT_CalcCRC32((uint32_t)&tmp, sizeof(tmp));

    if (!BSP_FLASH_Unlock()) return false;
    if (!BSP_FLASH_ErasePage(BOOT_SETTINGS1_ADDR)) { BSP_FLASH_Lock(); return false; }
    const uint16_t *p=(const uint16_t *)&tmp;
    for (uint32_t i=0;i<sizeof(tmp)/2U;i++)
        if (!BSP_FLASH_WriteHalfWord(BOOT_SETTINGS1_ADDR+i*2U,p[i])) { BSP_FLASH_Lock(); return false; }
    BSP_FLASH_Lock();

    if (!BSP_SETTINGS_Validate((const BOOT_Settings_t *)BOOT_SETTINGS1_ADDR)) return false;

    if (!BSP_FLASH_Unlock()) return false;
    if (!BSP_FLASH_ErasePage(BOOT_SETTINGS2_ADDR)) { BSP_FLASH_Lock(); return false; }
    for (uint32_t i=0;i<sizeof(tmp)/2U;i++)
        if (!BSP_FLASH_WriteHalfWord(BOOT_SETTINGS2_ADDR+i*2U,p[i])) { BSP_FLASH_Lock(); return false; }
    BSP_FLASH_Lock();
    return BSP_SETTINGS_Validate((const BOOT_Settings_t *)BOOT_SETTINGS2_ADDR);
}
