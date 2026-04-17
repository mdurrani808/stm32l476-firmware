#include "test_pwm_system.h"

#include "can_params.h"
#include "pwm.h"
#include "stm32l4xx_hal.h"

#define TEST_PWM_SYSTEM_SIGNAL_NAME "SCIENCE_DC_MOTOR_PCB_C.dc_motor_velocity_target_0"
#define TEST_PWM_SYSTEM_RAW_MAX     32767

static uint8_t s_inited = 0U;
static int32_t s_last_raw = -1;
static float s_last_duty_percent = -1.0f;

static void enable_gpio_clock(GPIO_TypeDef* port)
{
  if (port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
  else if (port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
  else if (port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
  else if (port == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
  else if (port == GPIOE) __HAL_RCC_GPIOE_CLK_ENABLE();
  else if (port == GPIOH) __HAL_RCC_GPIOH_CLK_ENABLE();
}

static uint32_t get_apb1_timer_clock_hz(void)
{
  uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
  uint32_t ppre1 = (RCC->CFGR & RCC_CFGR_PPRE1);
  bool apb1_div1 = (ppre1 == RCC_CFGR_PPRE1_DIV1);
  return apb1_div1 ? pclk1 : (pclk1 * 2u);
}

static float clampf_local(float x, float lo, float hi)
{
  if (x < lo) return lo;
  if (x > hi) return hi;
  return x;
}

static bool init_once(void)
{
  GPIO_InitTypeDef gpio = {0};

  enable_gpio_clock(TEST_PWM_SYSTEM_GPIO_PORT);

  gpio.Pin = TEST_PWM_SYSTEM_GPIO_PIN;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio.Alternate = TEST_PWM_SYSTEM_GPIO_AF;
  HAL_GPIO_Init(TEST_PWM_SYSTEM_GPIO_PORT, &gpio);

  PlatformPwm_t pwm =
  {
    .timer = TEST_PWM_SYSTEM_TIMER,
    .channel = TEST_PWM_SYSTEM_CHANNEL,
    .timer_clock_hz = (TEST_PWM_SYSTEM_TIMER_CLOCK_HZ != 0U)
      ? TEST_PWM_SYSTEM_TIMER_CLOCK_HZ
      : get_apb1_timer_clock_hz(),
    .pwm_hz = TEST_PWM_SYSTEM_PWM_FREQUENCY_HZ,
  };

  if (!Platform_PWM_Init(&pwm)) return false;
  s_inited = 1U;
  s_last_raw = -1;
  s_last_duty_percent = -1.0f;
  return true;
}

static bool write_from_raw_command(int32_t raw_cmd)
{
  if (raw_cmd < 0) raw_cmd = 0;
  if (raw_cmd > TEST_PWM_SYSTEM_RAW_MAX) raw_cmd = TEST_PWM_SYSTEM_RAW_MAX;

  float duty_percent = ((float)raw_cmd * 100.0f) / (float)TEST_PWM_SYSTEM_RAW_MAX;
  duty_percent = clampf_local(duty_percent,
                              TEST_PWM_SYSTEM_MIN_DUTY_PERCENT,
                              TEST_PWM_SYSTEM_MAX_DUTY_PERCENT);

  uint32_t period_us = 1000000U / TEST_PWM_SYSTEM_PWM_FREQUENCY_HZ;
  uint32_t pulse_us = (uint32_t)((duty_percent * (float)period_us / 100.0f) + 0.5f);

  PlatformPwm_t pwm =
  {
    .timer = TEST_PWM_SYSTEM_TIMER,
    .channel = TEST_PWM_SYSTEM_CHANNEL,
    .timer_clock_hz = 0U,
    .pwm_hz = TEST_PWM_SYSTEM_PWM_FREQUENCY_HZ,
  };

  if (!Platform_PWM_WriteMicroseconds(&pwm, (uint16_t)pulse_us)) return false;

  s_last_raw = raw_cmd;
  s_last_duty_percent = duty_percent;
  return true;
}

void test_pwm_system_controller(void)
{
  int32_t raw_cmd = 0;

  if (!s_inited)
  {
    if (!init_once()) return;
  }

  if (!CanParams_GetInt32(TEST_PWM_SYSTEM_SIGNAL_NAME, &raw_cmd)) return;
  if (raw_cmd == s_last_raw) return;

  (void)write_from_raw_command(raw_cmd);
}

void TestPwmSystem_Controller(void)
{
  test_pwm_system_controller();
}

bool TestPwmSystem_GetLastDutyPercent(float* out_duty_percent)
{
  if (out_duty_percent == NULL) return false;
  if (!s_inited) return false;
  *out_duty_percent = s_last_duty_percent;
  return true;
}

bool TestPwmSystem_GetLastCommandRaw(int32_t* out_command_raw)
{
  if (out_command_raw == NULL) return false;
  if (!s_inited) return false;
  *out_command_raw = s_last_raw;
  return true;
}
