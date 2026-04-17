#ifndef CAN_SYSTEM_H
#define CAN_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/* Round-robin scheduled controller.
 * Call this periodically from the main loop.
 */
void can_system_controller(void);

/* =========================
 * New public API
 * =========================
 *
 * CanSystem_Send() schedules exactly one transmit of a DBC-defined message.
 * It does NOT modify any CanParams values.
 *
 * Naming rules:
 *   - "MESSAGE"        : valid only for non-multiplexed messages
 *   - "MESSAGE.SIGNAL" : valid for non-mux and mux messages
 *
 * For mux messages, "MESSAGE.SIGNAL" schedules the mux page that SIGNAL
 * belongs to. (You guarantee there are no "always" signals in muxed messages.)
 *
 * Returns:
 *   - true  if the name maps to a schedulable message/page
 *   - false otherwise
 */
bool CanSystem_Send(const char* full_name);

/* Sends a raw standard 11-bit CAN data frame directly.
 *
 * Format:
 *   - "XXX#"                    : zero-length payload
 *   - "XXX#112233"              : 3-byte payload
 *   - "7FF#0011223344556677"    : 8-byte payload
 *
 * Rules:
 *   - standard IDs only (1 to 3 hex digits, 0x000..0x7FF)
 *   - payload must contain an even number of hex digits
 *   - payload length may be 0 to 16 hex digits (0 to 8 bytes)
 *   - frame is transmitted immediately and bypasses the DBC/CanParams layer
 */
bool CanSystem_SendRaw(const char* frame_str);

/* Debug helpers (no counters)
 * - full_name follows the same naming rules as CanSystem_Send()
 */
bool CanSystem_DebugGetLastRxTick(const char* full_name, uint32_t* out_tick);
bool CanSystem_DebugGetLastTxTick(const char* full_name, uint32_t* out_tick);

/* Returns true when the given standard ID is present in the configured RX
 * allowlist (or when the allowlist is empty and all IDs are allowed).
 * Useful when validating hardware filter configuration with a debugger.
 */
bool CanSystem_DebugIsStdIdAllowed(uint32_t std_id);

/* =========================
 * Deprecated legacy API
 * =========================
 *
 * These perform set+send. Keep for compatibility, prefer:
 *   CanParams_Set*() then CanSystem_Send().
 */
#if defined(__GNUC__)
#define CAN_DEPRECATED __attribute__((deprecated))
#else
#define CAN_DEPRECATED
#endif

bool CAN_DEPRECATED CanSystem_SetBool(const char* full_name, bool value);
bool CAN_DEPRECATED CanSystem_SetInt32(const char* full_name, int32_t value);
bool CAN_DEPRECATED CanSystem_SetFloat(const char* full_name, float value);

#ifdef __cplusplus
}
#endif

#endif /* CAN_SYSTEM_H */
