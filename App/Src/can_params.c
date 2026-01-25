#include "can_params.h"

/*
 * Stored parameters (one per integrated DBC signal).
 * Cooperative scheduler: no ISRs touching this right now, so plain volatile is enough.
 */
static volatile uint8_t s_set_led = 0;

bool CanParams_Get_SetLED(void)
{
  return (s_set_led != 0);
}

/* Internal setters (used only by CAN system). Keep them non-public. */
void CanParams__Set_SetLED(uint8_t v)
{
  s_set_led = (v ? 1U : 0U);
}
