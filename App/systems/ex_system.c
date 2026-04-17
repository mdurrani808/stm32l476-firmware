#include "ex_system.h"

#include <stdbool.h>
#include <stdint.h>

#include "can_params.h"
#include "can_system.h"
#include "project_config.h"
#include "main.h"

/*
 * CAN API example system
 * ----------------------
 * This controller is intentionally verbose and heavily commented so you
 * can single-step through each public API call and see what it does.
 *
 * Enable from main.c when teaching the API. It uses real names from the
 * shipped DBC whenever possible.
 */

static uint8_t s_inited = 0U;
static uint32_t s_last_tick = 0U;
static bool s_toggle = false;
static int32_t s_demo_angle_deg = 0;
static bool s_last_filter_check = false;
static uint32_t s_last_rx_tick = 0U;
static uint32_t s_last_tx_tick = 0U;
static bool s_last_event_snapshot = false;
static bool s_last_event_consumed = false;

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

void ex_system_controller(void)
{
  if (!s_inited)
  {
    s_inited = 1U;
    s_last_tick = HAL_GetTick();
  }

  if (!tick_elapsed(&s_last_tick, PROJECT_EXAMPLE_API_PERIOD_MS))
  {
    return;
  }

  s_toggle = !s_toggle;
  s_demo_angle_deg += 15;
  if (s_demo_angle_deg > 180)
  {
    s_demo_angle_deg = 0;
  }

  /* 1) Set/Get a BOOL parameter, then schedule the mux page that contains it. */
  (void)CanParams_SetBool("POWER_PCB_R.pcb_led_success", s_toggle);
  (void)CanParams_GetBool("POWER_PCB_R.pcb_led_success", &s_toggle);
  (void)CanSystem_Send("POWER_PCB_R.pcb_led_success");

  /* 2) Set/Get an INT32 parameter. Most of this DBC uses integer physical
   * values, so int32 is the normal path for position / velocity / status data.
   */
  (void)CanParams_SetInt32("SCIENCE_SERVO_PCB_R.servo_position_state_resp_0", s_demo_angle_deg);
  (void)CanParams_SetInt32("SCIENCE_SERVO_PCB_R.servo_velocity_state_resp_0", 0);
  (void)CanParams_GetInt32("SCIENCE_SERVO_PCB_R.servo_position_state_resp_0", &s_demo_angle_deg);
  (void)CanSystem_Send("SCIENCE_SERVO_PCB_R.servo_position_state_resp_0");

  /* 3) Read an EVENT parameter two different ways:
   *    - GetEvent() snapshots the current latched state without clearing it.
   *    - ProcEvent() snapshots and clears only if the event was asserted.
   */
  (void)CanParams_GetEvent("SCIENCE_SERVO_PCB_C.servo_state_req_event_0", &s_last_event_snapshot);
  (void)CanParams_ProcEvent("SCIENCE_SERVO_PCB_C.servo_state_req_event_0", &s_last_event_consumed);

  /* 4) SetEvent() lets software raise an event parameter locally. This is
   * useful for demos, test harnesses, or software-originated triggers.
   */
  (void)CanParams_SetEvent("SCIENCE_SERVO_PCB_C.servo_status_req_event_0", true);

  /* 5) Legacy compatibility wrappers: they still perform set+send in one call.
   * Keep them for older code, but prefer CanParams_Set*() + CanSystem_Send().
   */
  (void)CanSystem_SetBool("SCIENCE_SERVO_PCB_R.pcb_led_success", s_toggle);
  (void)CanSystem_SetInt32("SCIENCE_DC_MOTOR_PCB_R.dc_motor_status_0", s_toggle ? 1 : 0);

  /* 6) Debug helpers: useful when validating that RX traffic was accepted by
   * the hardware/software filter and that a page actually transmitted.
   */
  (void)CanSystem_DebugGetLastRxTick("SCIENCE_SERVO_PCB_C.servo_state_req_event_0", &s_last_rx_tick);
  (void)CanSystem_DebugGetLastTxTick("SCIENCE_SERVO_PCB_R.servo_position_state_resp_0", &s_last_tx_tick);
  s_last_filter_check = CanSystem_DebugIsStdIdAllowed(PROJECT_CAN_ID_SCIENCE_SERVO_PCB_C);

  /* 7) Float API note:
   * The shipped 4.13.2026 DBC does not currently define any factor/offset based
   * float signals, so CanParams_SetFloat/GetFloat and CanSystem_SetFloat are
   * shown below as a commented template instead of live code.
   */
#if 0
  float demo_float = 12.5f;
  (void)CanParams_SetFloat("YOUR_FLOAT_SIGNAL_HERE", demo_float);
  (void)CanParams_GetFloat("YOUR_FLOAT_SIGNAL_HERE", &demo_float);
  (void)CanSystem_SetFloat("YOUR_FLOAT_SIGNAL_HERE", demo_float);
#endif
}
