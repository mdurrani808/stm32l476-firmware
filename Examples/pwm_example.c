#include <stdint.h>

#include "pwm.h"
#include "stm32l4xx_hal.h"

/* Initializes TIM3 CH1 for a 50 Hz servo-style output and writes a neutral
 * 1500 us pulse.
 */
void Example_PwmServoPulse(void)
{
  uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
  uint32_t timclk = ((RCC->CFGR & RCC_CFGR_PPRE1) == RCC_CFGR_PPRE1_DIV1) ? pclk1 : (pclk1 * 2U);

  PlatformPwm_t pwm = {
    .timer = TIM3,
    .channel = 1,
    .timer_clock_hz = timclk,
    .pwm_hz = 50U,
  };

  (void)Platform_PWM_Init(&pwm);
  (void)Platform_PWM_WriteMicroseconds(&pwm, 1500U);
}
