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
    return settings != 0 &&
           settings->magic == BOOT_SETTINGS_MAGIC &&
           settings_crc_ok(settings);
}

static bool sequence_newer(uint32_t a, uint32_t b)
{
    return (int32_t)(a - b) > 0;
}

const BOOT_Settings_t *BSP_SETTINGS_GetActive(void)
{
    const BOOT_Settings_t *a = (const BOOT_Settings_t *)BOOT_SETTINGS1_ADDR;
    const BOOT_Settings_t *b = (const BOOT_Settings_t *)BOOT_SETTINGS2_ADDR;
    const bool a_ok = BSP_SETTINGS_Validate(a);
    const bool b_ok = BSP_SETTINGS_Validate(b);

    if (a_ok && b_ok)
        return sequence_newer(a->sequence, b->sequence) ? a : b;

    if (a_ok)
        return a;

    if (b_ok)
        return b;

    return 0;
}

bool BSP_SETTINGS_Store(const BOOT_Settings_t *settings)
{
    if (settings == 0)
        return false;

    BOOT_Settings_t tmp;
    memcpy(&tmp, settings, sizeof(tmp));
    tmp.data_crc32 = 0U;
    tmp.data_crc32 = BOOT_CalcCRC32((uint32_t)&tmp, sizeof(tmp));

    const BOOT_Settings_t *active = BSP_SETTINGS_GetActive();
    const uint32_t target_addr =
        (active == (const BOOT_Settings_t *)BOOT_SETTINGS1_ADDR)
            ? BOOT_SETTINGS2_ADDR
            : BOOT_SETTINGS1_ADDR;

    if (!BSP_FLASH_Unlock())
        return false;

    if (!BSP_FLASH_ErasePage(target_addr))
    {
        BSP_FLASH_Lock();
        return false;
    }

    const uint16_t *p = (const uint16_t *)&tmp;

    for (uint32_t i = 0U; i < sizeof(tmp) / 2U; ++i)
    {
        if (!BSP_FLASH_WriteHalfWord(target_addr + i * 2U, p[i]))
        {
            BSP_FLASH_Lock();
            return false;
        }
    }

    BSP_FLASH_Lock();
    return BSP_SETTINGS_Validate((const BOOT_Settings_t *)target_addr);
}
