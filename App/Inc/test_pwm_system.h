#ifndef TEST_PWM_SYSTEM_H
#define TEST_PWM_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

/*
 * Test PWM system
 * ---------------
 * Reads the DBC parameter:
 *   SCIENCE_DC_MOTOR_PCB_C.dc_motor_velocity_target_0
 *
 * Mapping:
 *   0     -> 0% duty cycle
 *   32767 -> 100% duty cycle
 *
 * The computed duty cycle is then clamped to the min/max duty settings below.
 * Leave the defaults at 0% and 100% if you want the raw full-range behavior.
 */

/* PWM output frequency in Hz. Example values:
 *   50    for servo-style pulses
 *   1000  for a simple visible/scope-friendly PWM demo
 *   20000 for many motor-driver style PWM tests
 */
#define TEST_PWM_SYSTEM_PWM_FREQUENCY_HZ      1000U

/* Duty-cycle clamp in percent.
 * The input command is first converted from 0..32767 into 0..100%, then
 * clamped into this range.
 */
#define TEST_PWM_SYSTEM_MIN_DUTY_PERCENT      0.0f
#define TEST_PWM_SYSTEM_MAX_DUTY_PERCENT      100.0f

/* Timer/pin selection.
 *
 * This system uses the same timer families already exercised elsewhere in this
 * repo: TIM2, TIM3, and TIM4. Pick a GPIO pin that matches the timer channel
 * and alternate function you configure below.
 *
 * Known pin mappings already used in this project and safe as starting points:
 *   PA15 -> TIM2_CH1 AF1
 *   PB3  -> TIM2_CH2 AF1
 *   PA3  -> TIM2_CH4 AF1
 *   PC6  -> TIM3_CH1 AF2
 *   PA7  -> TIM3_CH2 AF2
 *   PB0  -> TIM3_CH3 AF2
 *   PB7  -> TIM4_CH2 AF2
 *   PB8  -> TIM4_CH3 AF2
 *
 * Default below: PA15 / TIM2_CH1 / AF1
 */
#define TEST_PWM_SYSTEM_GPIO_PORT             GPIOA
#define TEST_PWM_SYSTEM_GPIO_PIN              GPIO_PIN_15
#define TEST_PWM_SYSTEM_GPIO_AF               GPIO_AF1_TIM2
#define TEST_PWM_SYSTEM_TIMER                 TIM2
#define TEST_PWM_SYSTEM_CHANNEL               1U

/* Timer input clock in Hz passed to Platform_PWM_Init().
 * For TIM2/TIM3/TIM4 on STM32L476 this is the APB1 timer clock.
 * Leave at 0 to auto-detect from RCC at runtime.
 */
#define TEST_PWM_SYSTEM_TIMER_CLOCK_HZ        0U

void test_pwm_system_controller(void);
void TestPwmSystem_Controller(void);

bool TestPwmSystem_GetLastDutyPercent(float* out_duty_percent);
bool TestPwmSystem_GetLastCommandRaw(int32_t* out_command_raw);

#ifdef __cplusplus
}
#endif

#endif /* TEST_PWM_SYSTEM_H */
