#include <stdbool.h>
#include <stdint.h>

#include "adc.h"

/* Example ADC1 channel read. Swap channel to match your board wiring. */
bool Example_ReadAnalog(uint16_t* out_raw)
{
  PlatformAdc_t adc = { .instance = ADC1, .channel = 5U };
  if (!Platform_ADC_InitSingleEnded(&adc))
  {
    return false;
  }

  return Platform_ADC_ReadRaw(&adc, out_raw);
}
