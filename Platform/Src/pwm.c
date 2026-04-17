
#include "pwm.h"
#include "stm32l4xx_hal.h"
static void enable_timer_clock(TIM_TypeDef* tim)
{
  if (tim == TIM2) __HAL_RCC_TIM2_CLK_ENABLE();
  else if (tim == TIM3) __HAL_RCC_TIM3_CLK_ENABLE();
  else if (tim == TIM4) __HAL_RCC_TIM4_CLK_ENABLE();
}
bool Platform_PWM_Init(PlatformPwm_t* pwm)
{
  if ((pwm == NULL) || (pwm->timer == NULL) || (pwm->channel < 1U) || (pwm->channel > 4U) || (pwm->pwm_hz == 0U)) return false;
  enable_timer_clock(pwm->timer);
  uint32_t tick_hz = 1000000U;
  uint32_t prescaler = pwm->timer_clock_hz / tick_hz;
  if (prescaler == 0U) prescaler = 1U;
  prescaler -= 1U;
  pwm->timer->CR1 = 0U;
  pwm->timer->PSC = (uint16_t)prescaler;
  pwm->timer->ARR = (tick_hz / pwm->pwm_hz) - 1U;
  pwm->timer->EGR = TIM_EGR_UG;
  pwm->timer->CR1 |= TIM_CR1_ARPE;
  switch (pwm->channel)
  {
    case 1: pwm->timer->CCMR1 = (pwm->timer->CCMR1 & ~(TIM_CCMR1_OC1M | TIM_CCMR1_OC1PE)) | (6U << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE; pwm->timer->CCER |= TIM_CCER_CC1E; break;
    case 2: pwm->timer->CCMR1 = (pwm->timer->CCMR1 & ~(TIM_CCMR1_OC2M | TIM_CCMR1_OC2PE)) | (6U << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE; pwm->timer->CCER |= TIM_CCER_CC2E; break;
    case 3: pwm->timer->CCMR2 = (pwm->timer->CCMR2 & ~(TIM_CCMR2_OC3M | TIM_CCMR2_OC3PE)) | (6U << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE; pwm->timer->CCER |= TIM_CCER_CC3E; break;
    case 4: pwm->timer->CCMR2 = (pwm->timer->CCMR2 & ~(TIM_CCMR2_OC4M | TIM_CCMR2_OC4PE)) | (6U << TIM_CCMR2_OC4M_Pos) | TIM_CCMR2_OC4PE; pwm->timer->CCER |= TIM_CCER_CC4E; break;
    default: return false;
  }
  pwm->timer->CR1 |= TIM_CR1_CEN;
  return true;
}
bool Platform_PWM_WriteMicroseconds(const PlatformPwm_t* pwm, uint16_t pulse_us)
{
  if ((pwm == NULL) || (pwm->timer == NULL)) return false;
  switch (pwm->channel)
  {
    case 1: pwm->timer->CCR1 = pulse_us; break;
    case 2: pwm->timer->CCR2 = pulse_us; break;
    case 3: pwm->timer->CCR3 = pulse_us; break;
    case 4: pwm->timer->CCR4 = pulse_us; break;
    default: return false;
  }
  return true;
}
