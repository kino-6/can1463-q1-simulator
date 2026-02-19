#ifndef FAULT_DETECTOR_H
#define FAULT_DETECTOR_H

#include "tcan1463q1_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize fault detector state
 * @param state Pointer to FaultState structure
 */
void fault_detector_init(FaultState* state);

/**
 * Update fault detection logic
 * @param state Pointer to FaultState structure
 * @param txd_low True if TXD pin is low (dominant)
 * @param rxd_low True if RXD pin is low (dominant)
 * @param bus_state Current CAN bus state
 * @param tj_temperature Junction temperature in Celsius
 * @param current_time Current simulation time in nanoseconds
 * @param mode Current operating mode
 */
void fault_detector_update(
    FaultState* state,
    bool txd_low,
    bool rxd_low,
    BusState bus_state,
    double tj_temperature,
    uint64_t current_time,
    OperatingMode mode
);

/**
 * Check for TXD clamped low before entering Normal mode
 * @param state Pointer to FaultState structure
 * @param txd_low True if TXD pin is low
 * @param entering_mode Mode being entered
 */
void fault_detector_check_txdclp(
    FaultState* state,
    bool txd_low,
    OperatingMode entering_mode
);

/**
 * Check for TXD dominant timeout
 * @param state Pointer to FaultState structure
 * @param txd_low True if TXD pin is low
 * @param current_time Current simulation time in nanoseconds
 */
void fault_detector_check_txddto(
    FaultState* state,
    bool txd_low,
    uint64_t current_time
);

/**
 * Check for TXD/RXD short
 * @param state Pointer to FaultState structure
 * @param txd_low True if TXD pin is low
 * @param rxd_low True if RXD pin is low
 * @param current_time Current simulation time in nanoseconds
 */
void fault_detector_check_txdrxd(
    FaultState* state,
    bool txd_low,
    bool rxd_low,
    uint64_t current_time
);

/**
 * Check for CAN bus dominant fault
 * @param state Pointer to FaultState structure
 * @param bus_state Current CAN bus state
 * @param current_time Current simulation time in nanoseconds
 */
void fault_detector_check_candom(
    FaultState* state,
    BusState bus_state,
    uint64_t current_time
);

/**
 * Check for thermal shutdown
 * @param state Pointer to FaultState structure
 * @param tj_temperature Junction temperature in Celsius
 */
void fault_detector_check_tsd(
    FaultState* state,
    double tj_temperature
);

/**
 * Check for CAN bus fault (4 consecutive transitions)
 * @param state Pointer to FaultState structure
 * @param bus_state Current CAN bus state
 * @param mode Current operating mode
 */
void fault_detector_check_cbf(
    FaultState* state,
    BusState bus_state,
    OperatingMode mode
);

/**
 * Check if any fault is active
 * @param state Pointer to FaultState structure
 * @return True if any fault flag is set
 */
bool fault_detector_has_any_fault(const FaultState* state);

/**
 * Get nFAULT pin state (active low)
 * @param state Pointer to FaultState structure
 * @return True if nFAULT should be low (fault present)
 */
bool fault_detector_get_nfault_state(const FaultState* state);

/**
 * Check if CAN driver should be disabled
 * @param state Pointer to FaultState structure
 * @return True if CAN driver should be disabled
 */
bool fault_detector_should_disable_driver(const FaultState* state);

#ifdef __cplusplus
}
#endif

#endif // FAULT_DETECTOR_H
