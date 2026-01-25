#include "pcb_led_system.h"

#include "app_config.h"
#include "can_params.h"
#include "main.h"

void PcbLedSystem_Init(void)
{
  /* Ensure LED starts OFF by default (or follow current param) */
  const bool on = CanParams_Get_SetLED();
  HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void PcbLedSystem_Tick(void)
{
  const bool on = CanParams_Get_SetLED();
  HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
