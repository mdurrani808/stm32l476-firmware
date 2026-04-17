
#ifndef PLATFORM_PWM_H
#define PLATFORM_PWM_H
#include <stdbool.h>
#include <stdint.h>
#include "stm32l476xx.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct
{
  TIM_TypeDef* timer;
  uint8_t channel;
  uint32_t timer_clock_hz;
  uint32_t pwm_hz;
} PlatformPwm_t;
bool Platform_PWM_Init(PlatformPwm_t* pwm);
bool Platform_PWM_WriteMicroseconds(const PlatformPwm_t* pwm, uint16_t pulse_us);
#ifdef __cplusplus
}
#endif
#endif
