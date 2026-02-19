#ifndef POWER_MONITOR_H
#define POWER_MONITOR_H

#include "tcan1463q1_types.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Power Monitor - Monitors supply voltages and detects undervoltage conditions
 */

/**
 * Initialize power state structure
 * @param state Pointer to power state structure
 */
void power_monitor_init(PowerState* state);

/**
 * Update power monitor state based on current voltages and time
 * @param state Pointer to power state structure
 * @param vsup VSUP voltage
 * @param vcc VCC voltage
 * @param vio VIO voltage
 * @param current_time Current simulation time in nanoseconds
 */
void power_monitor_update(PowerState* state, double vsup, double vcc, 
                         double vio, uint64_t current_time);

/**
 * Check if VSUP is valid (above rising threshold)
 * @param state Pointer to power state structure
 * @return true if VSUP is valid
 */
bool power_monitor_is_vsup_valid(const PowerState* state);

/**
 * Check if VCC is valid (above rising threshold)
 * @param state Pointer to power state structure
 * @return true if VCC is valid
 */
bool power_monitor_is_vcc_valid(const PowerState* state);

/**
 * Check if VIO is valid (above rising threshold)
 * @param state Pointer to power state structure
 * @return true if VIO is valid
 */
bool power_monitor_is_vio_valid(const PowerState* state);

/**
 * Get UVSUP flag state
 * @param state Pointer to power state structure
 * @return true if UVSUP flag is set
 */
bool power_monitor_get_uvsup_flag(const PowerState* state);

/**
 * Get UVCC flag state
 * @param state Pointer to power state structure
 * @return true if UVCC flag is set
 */
bool power_monitor_get_uvcc_flag(const PowerState* state);

/**
 * Get UVIO flag state
 * @param state Pointer to power state structure
 * @return true if UVIO flag is set
 */
bool power_monitor_get_uvio_flag(const PowerState* state);

/**
 * Get PWRON flag state
 * @param state Pointer to power state structure
 * @return true if PWRON flag is set
 */
bool power_monitor_get_pwron_flag(const PowerState* state);

/**
 * Clear PWRON flag (typically done when entering Normal mode)
 * @param state Pointer to power state structure
 */
void power_monitor_clear_pwron_flag(PowerState* state);

#ifdef __cplusplus
}
#endif

#endif // POWER_MONITOR_H
