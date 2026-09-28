#ifndef __STM32F303xC_H
#define __STM32F303xC_H
/*
 * STM32F303xC compatibility device header.
 * The register map is shared with the STM32F303x8 CMSIS device header.
 * The project target is STM32F303CBT6 and the linker defines the 128 KiB
 * flash geometry. The Cortex-M4 MPU is enabled on the xC line.
 */
#include "stm32f303x8.h"
#ifdef __MPU_PRESENT
#undef __MPU_PRESENT
#endif
#define __MPU_PRESENT 1U
#endif
