#ifndef CAN_TRANSCEIVER_H
#define CAN_TRANSCEIVER_H

#include "tcan1463q1_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize CAN transceiver
 * @param transceiver Pointer to CANTransceiver structure
 */
void can_transceiver_init(CANTransceiver* transceiver);

/**
 * Update CAN transceiver state based on operating mode and inputs
 * @param transceiver Pointer to CANTransceiver structure
 * @param mode Current operating mode
 * @param txd_low TXD input state (true if low/dominant)
 * @param canh_voltage External CANH voltage
 * @param canl_voltage External CANL voltage
 * @param current_time Current simulation time in nanoseconds
 */
void can_transceiver_update(
    CANTransceiver* transceiver,
    OperatingMode mode,
    bool txd_low,
    double canh_voltage,
    double canl_voltage,
    uint64_t current_time
);

/**
 * Get bus state from differential voltage
 * @param vdiff Differential voltage (CANH - CANL)
 * @return Bus state (dominant, recessive, or indeterminate)
 */
BusState can_transceiver_get_bus_state(double vdiff);

/**
 * Drive CAN bus (set CANH/CANL voltages)
 * @param transceiver Pointer to CANTransceiver structure
 * @param dominant True to drive dominant, false for recessive
 * @param canh Output CANH voltage
 * @param canl Output CANL voltage
 */
void can_transceiver_drive_bus(
    CANTransceiver* transceiver,
    bool dominant,
    double* canh,
    double* canl
);

/**
 * Update RXD output based on bus state with propagation delay
 * @param transceiver Pointer to CANTransceiver structure
 * @param bus_state Current bus state
 * @param current_time Current simulation time in nanoseconds (for applying pending updates)
 * @param schedule_time Time to use for scheduling new updates (typically time before step)
 */
void can_transceiver_update_rxd(
    CANTransceiver* transceiver,
    BusState bus_state,
    uint64_t current_time,
    uint64_t schedule_time
);

/**
 * Update CAN transceiver state machine
 * @param transceiver Pointer to CANTransceiver structure
 * @param mode Current operating mode
 * @param bus_state Current bus state
 * @param vsup_valid True if VSUP is valid
 * @param current_time Current simulation time in nanoseconds
 */
void can_transceiver_update_state_machine(
    CANTransceiver* transceiver,
    OperatingMode mode,
    BusState bus_state,
    bool vsup_valid,
    uint64_t current_time
);

#ifdef __cplusplus
}
#endif

#endif // CAN_TRANSCEIVER_H
