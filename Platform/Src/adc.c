
#include "adc.h"
#include "stm32l4xx_hal.h"
bool Platform_ADC_InitSingleEnded(PlatformAdc_t* adc)
{
  if ((adc == NULL) || (adc->instance != ADC1)) return false;
  __HAL_RCC_ADC_CLK_ENABLE();
  ADC123_COMMON->CCR = 0U;
  adc->instance->CR &= ~ADC_CR_DEEPPWD;
  adc->instance->CR |= ADC_CR_ADVREGEN;
  for (volatile uint32_t i = 0; i < 10000U; ++i) { __NOP(); }
  adc->instance->CR &= ~ADC_CR_ADEN;
  adc->instance->CFGR = 0U;
  adc->instance->SQR1 = (adc->channel << ADC_SQR1_SQ1_Pos);
  adc->instance->SMPR1 = ADC_SMPR1_SMP5_2;
  adc->instance->CR |= ADC_CR_ADCAL;
  while ((adc->instance->CR & ADC_CR_ADCAL) != 0U) {}
  adc->instance->ISR |= ADC_ISR_ADRDY;
  adc->instance->CR |= ADC_CR_ADEN;
  while ((adc->instance->ISR & ADC_ISR_ADRDY) == 0U) {}
  return true;
}
bool Platform_ADC_ReadRaw(const PlatformAdc_t* adc, uint16_t* out_raw)
{
  if ((adc == NULL) || (adc->instance == NULL) || (out_raw == NULL)) return false;
  adc->instance->CR |= ADC_CR_ADSTART;
  while ((adc->instance->ISR & ADC_ISR_EOC) == 0U) {}
  *out_raw = (uint16_t)(adc->instance->DR & 0xFFFFU);
  return true;
}
