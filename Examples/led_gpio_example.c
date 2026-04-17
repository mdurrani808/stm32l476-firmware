#include <stdbool.h>

#include "app_config.h"
#include "stm32l4xx_hal.h"

void Example_LedOn(void)
{
  HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, GPIO_PIN_SET);
}

void Example_LedOff(void)
{
  HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, GPIO_PIN_RESET);
}

void Example_LedMirrorBool(bool on)
{
  HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
