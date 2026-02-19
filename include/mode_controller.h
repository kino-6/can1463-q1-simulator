#ifndef MODE_CONTROLLER_H
#define MODE_CONTROLLER_H

#include "tcan1463q1_types.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Mode Controller - Controls operating mode transitions
 */

/**
 * Initialize mode state structure
 * @param state Pointer to mode state structure
 */
void mode_controller_init(ModeState* state);

/**
 * Update mode based on pin states, power conditions, and flags
 * @param state Pointer to mode state structure
 * @param en_high EN pin is high
 * @param nstb_high nSTB pin is high
 * @param vsup_valid VSUP is above valid threshold
 * @param wakerq_set WAKERQ flag is set
 * @param current_time Current simulation time in nanoseconds
 * @return New operating mode
 */
OperatingMode mode_controller_update(
    ModeState* state,
    bool en_high,
    bool nstb_high,
    bool vsup_valid,
    bool wakerq_set,
    uint64_t current_time
);

/**
 * Check if mode transition is valid
 * @param from Source mode
 * @param to Target mode
 * @return true if transition is valid
 */
bool mode_controller_can_transition(OperatingMode from, OperatingMode to);

/**
 * Get current operating mode
 * @param state Pointer to mode state structure
 * @return Current operating mode
 */
OperatingMode mode_controller_get_mode(const ModeState* state);

/**
 * Get previous operating mode
 * @param state Pointer to mode state structure
 * @return Previous operating mode
 */
OperatingMode mode_controller_get_previous_mode(const ModeState* state);

/**
 * Get time elapsed in current mode (in nanoseconds)
 * @param state Pointer to mode state structure
 * @param current_time Current simulation time in nanoseconds
 * @return Time elapsed in current mode
 */
uint64_t mode_controller_get_time_in_mode(const ModeState* state, uint64_t current_time);

/**
 * Set WAKERQ flag
 * @param state Pointer to mode state structure
 * @param set true to set flag, false to clear
 */
void mode_controller_set_wakerq(ModeState* state, bool set);

/**
 * Get WAKERQ flag
 * @param state Pointer to mode state structure
 * @return true if WAKERQ flag is set
 */
bool mode_controller_get_wakerq(const ModeState* state);

#ifdef __cplusplus
}
#endif

#endif // MODE_CONTROLLER_H
