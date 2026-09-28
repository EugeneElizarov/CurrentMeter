#ifndef __STM32F303xC_H
#define __STM32F303xC_H
#include "stm32f303x8.h"
#ifdef __MPU_PRESENT
#undef __MPU_PRESENT
#endif
#define __MPU_PRESENT 1U

typedef struct
{
  __IO uint16_t EP0R; __IO uint16_t RESERVED0;
  __IO uint16_t EP1R; __IO uint16_t RESERVED1;
  __IO uint16_t EP2R; __IO uint16_t RESERVED2;
  __IO uint16_t EP3R; __IO uint16_t RESERVED3;
  __IO uint16_t EP4R; __IO uint16_t RESERVED4;
  __IO uint16_t EP5R; __IO uint16_t RESERVED5;
  __IO uint16_t EP6R; __IO uint16_t RESERVED6;
  __IO uint16_t EP7R; __IO uint16_t RESERVED7;
  uint16_t RESERVED8[16];
  __IO uint16_t CNTR; __IO uint16_t RESERVED9;
  __IO uint16_t ISTR; __IO uint16_t RESERVED10;
  __IO uint16_t FNR; __IO uint16_t RESERVED11;
  __IO uint16_t DADDR; __IO uint16_t RESERVED12;
  __IO uint16_t BTABLE; __IO uint16_t RESERVED13;
  __IO uint16_t LPMCSR; __IO uint16_t RESERVED14;
  __IO uint16_t BCDR; __IO uint16_t RESERVED15;
} USB_TypeDef;

#define USB_BASE ((uint32_t)0x40005C00U)
#define USB ((USB_TypeDef *)USB_BASE)
#define USB_PMA_BASE ((uint32_t)0x40006000U)
#define USB_PMA_ACCESS 2U
#endif
