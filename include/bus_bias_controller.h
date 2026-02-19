#ifndef BUS_BIAS_CONTROLLER_H
#define BUS_BIAS_CONTROLLER_H

#include "tcan1463q1_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize bus bias controller
 * @param controller Pointer to BusBiasController structure
 */
void bus_bias_controller_init(BusBiasController* controller);

/**
 * Update bus bias state based on CAN transceiver state and bus activity
 * @param controller Pointer to BusBiasController structure
 * @param can_state Current CAN transceiver state
 * @param bus_state Current bus state
 * @param current_time Current simulation time in nanoseconds
 */
void bus_bias_controller_update(
    BusBiasController* controller,
    CANTransceiverState can_state,
    BusState bus_state,
    uint64_t current_time
);

/**
 * Get bias voltages for CANH and CANL
 * @param controller Pointer to BusBiasController structure
 * @param vcc VCC supply voltage
 * @param canh Output CANH bias voltage
 * @param canl Output CANL bias voltage
 */
void bus_bias_controller_get_bias(
    const BusBiasController* controller,
    double vcc,
    double* canh,
    double* canl
);

/**
 * Check if bus has been inactive for tSILENCE duration
 * @param controller Pointer to BusBiasController structure
 * @param current_time Current simulation time in nanoseconds
 * @return True if silence timeout has occurred
 */
bool bus_bias_controller_is_silence_timeout(
    const BusBiasController* controller,
    uint64_t current_time
);

#ifdef __cplusplus
}
#endif

#endif // BUS_BIAS_CONTROLLER_H
