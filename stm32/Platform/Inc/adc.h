
#ifndef PLATFORM_ADC_H
#define PLATFORM_ADC_H
#include <stdbool.h>
#include <stdint.h>
#include "stm32l476xx.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct
{
  ADC_TypeDef* instance;
  uint32_t channel;
} PlatformAdc_t;
bool Platform_ADC_InitSingleEnded(PlatformAdc_t* adc);
bool Platform_ADC_ReadRaw(const PlatformAdc_t* adc, uint16_t* out_raw);
#ifdef __cplusplus
}
#endif
#endif
