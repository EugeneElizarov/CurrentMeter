#ifndef BSP_SETTINGS_H
#define BSP_SETTINGS_H

#include <stdbool.h>
#include "bootloader_defs.h"

bool BSP_SETTINGS_Validate(const BOOT_Settings_t *settings);
const BOOT_Settings_t *BSP_SETTINGS_GetActive(void);
bool BSP_SETTINGS_Store(const BOOT_Settings_t *settings);

#endif
