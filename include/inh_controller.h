#ifndef INH_CONTROLLER_H
#define INH_CONTROLLER_H

#include "tcan1463q1_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * INH controller state structure
 */
typedef struct {
    bool inh_enabled;           // INH functionality enabled (controlled by INH_MASK)
    bool inh_output_high;       // INH output state (true=high, false=high-Z)
    uint64_t wake_event_time;   // Timestamp of last wake event
    bool pending_inh_assertion; // INH assertion pending after wake-up delay
} INHController;

/**
 * Initialize INH controller
 * @param controller Pointer to INHController structure
 */
void inh_controller_init(INHController* controller);

/**
 * Update INH controller state
 * @param controller Pointer to INHController structure
 * @param mode Current operating mode
 * @param inh_mask_high INH_MASK pin state (true if high, false if low/floating)
 * @param wake_event True if a wake-up event just occurred
 * @param current_time Current simulation time in nanoseconds
 */
void inh_controller_update(
    INHController* controller,
    OperatingMode mode,
    bool inh_mask_high,
    bool wake_event,
    uint64_t current_time
);

/**
 * Get INH pin state
 * @param controller Pointer to INHController structure
 * @param state Output pin state (HIGH or HIGH_IMPEDANCE)
 * @param voltage Output voltage (VSUP - 0.5V to 1V when high)
 */
void inh_controller_get_pin_state(
    const INHController* controller,
    PinState* state,
    double* voltage
);

#ifdef __cplusplus
}
#endif

#endif // INH_CONTROLLER_H
