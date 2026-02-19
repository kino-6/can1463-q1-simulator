#ifndef WAKE_HANDLER_H
#define WAKE_HANDLER_H

#include "tcan1463q1_types.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Wake Handler - Handles remote and local wake-up events
 */

/**
 * Initialize wake state structure
 * @param state Pointer to wake state structure
 */
void wake_handler_init(WakeState* state);

/**
 * Update wake handler based on bus state, WAKE pin, and operating mode
 * @param state Pointer to wake state structure
 * @param bus_state Current CAN bus state
 * @param wake_pin_high WAKE pin state (true = high, false = low)
 * @param mode Current operating mode
 * @param current_time Current simulation time in nanoseconds
 */
void wake_handler_update(WakeState* state, BusState bus_state, 
                        bool wake_pin_high, OperatingMode mode,
                        uint64_t current_time);

/**
 * Process WUP (Wake-Up Pattern) on CAN bus
 * @param state Pointer to wake state structure
 * @param bus_state Current CAN bus state
 * @param current_time Current simulation time in nanoseconds
 */
void wake_handler_process_wup(WakeState* state, BusState bus_state,
                              uint64_t current_time);

/**
 * Process LWU (Local Wake-Up) from WAKE pin
 * @param state Pointer to wake state structure
 * @param wake_pin_high WAKE pin state (true = high, false = low)
 * @param current_time Current simulation time in nanoseconds
 */
void wake_handler_process_lwu(WakeState* state, bool wake_pin_high,
                              uint64_t current_time);

/**
 * Clear wake-up flags (typically when entering Normal mode)
 * @param state Pointer to wake state structure
 */
void wake_handler_clear_flags(WakeState* state);

/**
 * Get WAKERQ flag state
 * @param state Pointer to wake state structure
 * @return true if WAKERQ flag is set
 */
bool wake_handler_get_wakerq(const WakeState* state);

/**
 * Get WAKESR flag state
 * @param state Pointer to wake state structure
 * @return true if WAKESR flag is set
 */
bool wake_handler_get_wakesr(const WakeState* state);

/**
 * Check if wake source is local (WAKE pin) or remote (CAN bus)
 * @param state Pointer to wake state structure
 * @return true if local wake-up, false if remote wake-up
 */
bool wake_handler_is_local_wake(const WakeState* state);

#ifdef __cplusplus
}
#endif

#endif // WAKE_HANDLER_H
