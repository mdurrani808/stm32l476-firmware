#ifndef CAN_PARAMS_H
#define CAN_PARAMS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*
 * CAN Parameters (derived from DBC signals)
 *
 * For now: only STEPPER_COMMAND (ID 128 / 0x80) :: Set_LED
 */

/* Returns the latest decoded value of STEPPER_COMMAND.Set_LED */
bool CanParams_Get_SetLED(void);

#ifdef __cplusplus
}
#endif

#endif /* CAN_PARAMS_H */
