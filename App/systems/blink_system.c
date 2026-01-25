#include "blink_system.h"

#include "app_config.h"
#include "main.h"

static uint32_t s_last_toggle_ms = 0;

void BlinkSystem_Init(void)
{
  s_last_toggle_ms = HAL_GetTick();
}

void BlinkSystem_Tick(void)
{
#if (SYSTEM_BLINK_ENABLED)
  const uint32_t now = HAL_GetTick();

  if ((now - s_last_toggle_ms) >= BLINK_PERIOD_MS)
  {
    s_last_toggle_ms = now;
    HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);
  }
#else
  /* Blink system disabled: do nothing (still compiled, but inert). */
  (void)s_last_toggle_ms;
#endif
}
