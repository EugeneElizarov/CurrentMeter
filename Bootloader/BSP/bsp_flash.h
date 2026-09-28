#ifndef BSP_FLASH_H
#define BSP_FLASH_H

#include <stdint.h>
#include <stdbool.h>

bool BSP_FLASH_Unlock(void);
void BSP_FLASH_Lock(void);
bool BSP_FLASH_ErasePage(uint32_t addr);
bool BSP_FLASH_WriteHalfWord(uint32_t addr, uint16_t data);
bool BSP_FLASH_WriteWord(uint32_t addr, uint32_t data);

#endif
