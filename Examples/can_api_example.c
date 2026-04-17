#include <stdbool.h>
#include <stdint.h>

#include "can_params.h"
#include "can_system.h"

/* Send a POWER_PCB command-page value. */
void Example_CanApi_CommandPowerLed(bool on)
{
  (void)CanParams_SetBool("POWER_PCB_C.pcb_led_status", on);
  (void)CanSystem_Send("POWER_PCB_C.pcb_led_status");
}

/* Send a SCIENCE_SERVO command-page value. */
void Example_CanApi_SendServoPosition0(int32_t degrees)
{
  (void)CanParams_SetInt32("SCIENCE_SERVO_PCB_C.servo_position_target_0", degrees);
  (void)CanSystem_Send("SCIENCE_SERVO_PCB_C.servo_position_target_0");
}

/* Request a muxed state page from the SCIENCE_SERVO response message. */
void Example_CanApi_RequestServoState0(void)
{
  (void)CanParams_SetBool("SCIENCE_SERVO_PCB_C.servo_state_req_event_0", true);
  (void)CanSystem_Send("SCIENCE_SERVO_PCB_C.servo_state_req_event_0");
  (void)CanParams_SetBool("SCIENCE_SERVO_PCB_C.servo_state_req_event_0", false);
}

/* Publish a muxed SCIENCE_SERVO response page locally. */
void Example_CanApi_PublishServoStateResponse0(int32_t pos_deg, int32_t vel_deg_s)
{
  (void)CanParams_SetInt32("SCIENCE_SERVO_PCB_R.servo_position_state_resp_0", pos_deg);
  (void)CanParams_SetInt32("SCIENCE_SERVO_PCB_R.servo_velocity_state_resp_0", vel_deg_s);
  (void)CanSystem_Send("SCIENCE_SERVO_PCB_R.servo_position_state_resp_0");
}

/* Publish a SCIENCE_DC_MOTOR spec page locally. */
void Example_CanApi_PublishDcMotorSpec0(int32_t velocity_max_deg_s)
{
  (void)CanParams_SetInt32("SCIENCE_DC_MOTOR_PCB_R.dc_motor_velocity_max_0", velocity_max_deg_s);
  (void)CanSystem_Send("SCIENCE_DC_MOTOR_PCB_R.dc_motor_velocity_max_0");
}
