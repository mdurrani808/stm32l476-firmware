#ifndef CAN_PARAMS_H
#define CAN_PARAMS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/* =========================
 *  CAN parameter data model
 * =========================
 *
 * Parameters are named by the DBC "MESSAGE.SIGNAL" convention.
 * Two additional global parameters are always present:
 *   - "pending_inbox"  (bool)
 *   - "pending_outbox" (bool)
 */

typedef enum
{
  CANP_TYPE_BOOL = 0,
  CANP_TYPE_INT32,
  CANP_TYPE_FLOAT
} canp_type_t;

/* =========================
 *  External read API
 * ========================= */

bool CanParams_IsValid(const char* full_name);

bool CanParams_GetBool(const char* full_name, bool* out_value);
bool CanParams_GetInt32(const char* full_name, int32_t* out_value);
bool CanParams_GetFloat(const char* full_name, float* out_value);

/*
 * Generic write API (used by CAN system TX helpers).
 *
 * NOTE: For DBC-defined signals, application code should prefer calling
 * CanSystem_SetBool/Int32/Float (declared in can_system.h) so that the
 * CAN system will transmit the relevant message on its next tick.
 */
bool CanParams_SetBool(const char* full_name, bool value);
bool CanParams_SetInt32(const char* full_name, int32_t value);
bool CanParams_SetFloat(const char* full_name, float value);

/* =========================
 *  CAN-system internal API
 * =========================
 *
 * These are called by the CAN system during DBC parse and RX decode.
 */

void CanParams__Reset(void);
bool CanParams__Create(const char* full_name, canp_type_t type);

bool CanParams__UpdateBool(const char* full_name, uint8_t value);
bool CanParams__UpdateInt32(const char* full_name, int32_t value);
bool CanParams__UpdateFloat(const char* full_name, float value);

#ifdef __cplusplus
}
#endif

#endif /* CAN_PARAMS_H */
