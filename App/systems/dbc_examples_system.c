#include "dbc_examples_system.h"

#include <stdbool.h>
#include <stdint.h>

#include "can_params.h"
#include "can_system.h"
#include "project_config.h"
#include "main.h"

/*
 * DBC functional example system
 * -----------------------------
 * Enable this controller from main.c when you want a self-contained demo that
 * exercises realistic pages from the shipped DBC:
 *   - POWER_PCB_C / POWER_PCB_R
 *   - SCIENCE_SERVO_PCB_C / SCIENCE_SERVO_PCB_R
 *   - SCIENCE_DC_MOTOR_PCB_C / SCIENCE_DC_MOTOR_PCB_R
 *
 * This system is intended for CAN-tool bring-up. It mirrors requests into
 * sensible response pages so you can confirm mux handling, parameter updates,
 * and message scheduling without adding your own application logic first.
 */

static uint8_t s_inited = 0U;
static uint32_t s_last_power_heartbeat_tick = 0U;
static uint32_t s_last_status_tick = 0U;
static uint32_t s_last_spec_tick = 0U;

static bool tick_elapsed(uint32_t* stamp, uint32_t period_ms)
{
  uint32_t now = HAL_GetTick();
  if ((uint32_t)(now - *stamp) >= period_ms)
  {
    *stamp = now;
    return true;
  }
  return false;
}

static void publish_power_examples(void)
{
  bool led_on = false;
  if (CanParams_GetBool("POWER_PCB_C.pcb_led_status", &led_on))
  {
    (void)CanParams_SetBool("POWER_PCB_R.pcb_led_success", led_on);
    (void)CanSystem_Send("POWER_PCB_R.pcb_led_success");
  }

  int32_t hb_s = 0;
  if (CanParams_GetInt32("POWER_PCB_C.pcb_heartbeat_delay", &hb_s) && hb_s > 0)
  {
    if (tick_elapsed(&s_last_power_heartbeat_tick, (uint32_t)hb_s * 1000U))
    {
      (void)CanParams_SetBool("POWER_PCB_R.pcb_heartbeat_success", true);
      (void)CanSystem_Send("POWER_PCB_R.pcb_heartbeat_success");
    }
  }
}

static void publish_servo_examples(void)
{
  int32_t target_deg = 0;
  if (CanParams_GetInt32("SCIENCE_SERVO_PCB_C.servo_position_target_0", &target_deg))
  {
    (void)CanParams_SetInt32("SCIENCE_SERVO_PCB_R.servo_position_pos_resp_0", target_deg);
    (void)CanParams_SetInt32("SCIENCE_SERVO_PCB_R.servo_velocity_pos_resp_0", 0);
    (void)CanSystem_Send("SCIENCE_SERVO_PCB_R.servo_position_pos_resp_0");
  }

  bool req = false;
  if (CanParams_ProcEvent("SCIENCE_SERVO_PCB_C.servo_state_req_event_0", &req) && req)
  {
    (void)CanParams_SetInt32("SCIENCE_SERVO_PCB_R.servo_position_state_resp_0", target_deg);
    (void)CanParams_SetInt32("SCIENCE_SERVO_PCB_R.servo_velocity_state_resp_0", 0);
    (void)CanSystem_Send("SCIENCE_SERVO_PCB_R.servo_position_state_resp_0");
  }

  req = false;
  if (CanParams_ProcEvent("SCIENCE_SERVO_PCB_C.servo_status_req_event_0", &req) && req)
  {
    (void)CanParams_SetInt32("SCIENCE_SERVO_PCB_R.servo_status_0", 1);
    (void)CanSystem_Send("SCIENCE_SERVO_PCB_R.servo_status_0");
  }

  req = false;
  if (CanParams_ProcEvent("SCIENCE_SERVO_PCB_C.servo_spec_req_event_0", &req) && req)
  {
    (void)CanParams_SetInt32("SCIENCE_SERVO_PCB_R.servo_type_0", 1);
    (void)CanParams_SetInt32("SCIENCE_SERVO_PCB_R.servo_position_max_0", 197);
    (void)CanParams_SetInt32("SCIENCE_SERVO_PCB_R.servo_velocity_max_0", 250);
    (void)CanSystem_Send("SCIENCE_SERVO_PCB_R.servo_type_0");
  }
}

static void publish_dc_motor_examples(void)
{
  int32_t pos_target = 0;
  if (CanParams_GetInt32("SCIENCE_DC_MOTOR_PCB_C.dc_motor_position_target_0", &pos_target))
  {
    (void)CanParams_SetInt32("SCIENCE_DC_MOTOR_PCB_R.dc_motor_position_pos_resp_0", pos_target);
    (void)CanParams_SetInt32("SCIENCE_DC_MOTOR_PCB_R.dc_motor_velocity_pos_resp_0", 0);
    (void)CanSystem_Send("SCIENCE_DC_MOTOR_PCB_R.dc_motor_position_pos_resp_0");
  }

  bool req = false;
  if (CanParams_ProcEvent("SCIENCE_DC_MOTOR_PCB_C.dc_motor_state_req_event_0", &req) && req)
  {
    (void)CanParams_SetInt32("SCIENCE_DC_MOTOR_PCB_R.dc_motor_position_state_resp_0", pos_target);
    (void)CanParams_SetInt32("SCIENCE_DC_MOTOR_PCB_R.dc_motor_velocity_state_resp_0", 0);
    (void)CanSystem_Send("SCIENCE_DC_MOTOR_PCB_R.dc_motor_position_state_resp_0");
  }

  req = false;
  if (CanParams_ProcEvent("SCIENCE_DC_MOTOR_PCB_C.dc_motor_status_req_event_0", &req) && req)
  {
    (void)CanParams_SetInt32("SCIENCE_DC_MOTOR_PCB_R.dc_motor_status_0", 1);
    (void)CanSystem_Send("SCIENCE_DC_MOTOR_PCB_R.dc_motor_status_0");
  }

  req = false;
  if (CanParams_ProcEvent("SCIENCE_DC_MOTOR_PCB_C.dc_motor_spec_req_event_0", &req) && req)
  {
    (void)CanParams_SetInt32("SCIENCE_DC_MOTOR_PCB_R.dc_motor_velocity_max_0", 180);
    (void)CanSystem_Send("SCIENCE_DC_MOTOR_PCB_R.dc_motor_velocity_max_0");
  }
}

void dbc_examples_system_controller(void)
{
  if (!s_inited)
  {
    s_inited = 1U;
    s_last_power_heartbeat_tick = HAL_GetTick();
    s_last_status_tick = HAL_GetTick();
    s_last_spec_tick = HAL_GetTick();
  }

  publish_power_examples();
  publish_servo_examples();
  publish_dc_motor_examples();

  /* Optional periodic traffic so a CAN analyzer sees examples even when no
   * request events are being injected.
   */
  if (tick_elapsed(&s_last_status_tick, PROJECT_EXAMPLE_STATUS_PERIOD_MS))
  {
    (void)CanParams_SetInt32("SCIENCE_SERVO_PCB_R.servo_status_0", 1);
    (void)CanSystem_Send("SCIENCE_SERVO_PCB_R.servo_status_0");

    (void)CanParams_SetInt32("SCIENCE_DC_MOTOR_PCB_R.dc_motor_status_0", 1);
    (void)CanSystem_Send("SCIENCE_DC_MOTOR_PCB_R.dc_motor_status_0");
  }

  if (tick_elapsed(&s_last_spec_tick, PROJECT_EXAMPLE_SPEC_PERIOD_MS))
  {
    (void)CanParams_SetInt32("SCIENCE_SERVO_PCB_R.servo_type_0", 1);
    (void)CanParams_SetInt32("SCIENCE_SERVO_PCB_R.servo_position_max_0", 197);
    (void)CanParams_SetInt32("SCIENCE_SERVO_PCB_R.servo_velocity_max_0", 250);
    (void)CanSystem_Send("SCIENCE_SERVO_PCB_R.servo_type_0");
  }
}
