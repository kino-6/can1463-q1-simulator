#include "bus_bias_controller.h"
#include <string.h>

// Silence timeout for autonomous state transition (in nanoseconds)
// Using middle of range: 0.6-1.2s -> 0.9s
#define TSILENCE_NS (900000000ULL)

// Bias voltage levels
#define BIAS_VOLTAGE_2_5V 2.5
#define BIAS_VOLTAGE_GND 0.0

void bus_bias_controller_init(BusBiasController* controller) {
    if (!controller) return;
    
    memset(controller, 0, sizeof(BusBiasController));
    controller->state = BIAS_STATE_OFF;
    controller->last_bus_activity = 0;
}

void bus_bias_controller_update(
    BusBiasController* controller,
    CANTransceiverState can_state,
    BusState bus_state,
    uint64_t current_time
) {
    if (!controller) return;
    
    // Track bus activity for silence timeout
    if (bus_state == BUS_STATE_DOMINANT) {
        controller->last_bus_activity = current_time;
    }
    
    // Initialize last_bus_activity if it's zero
    if (controller->last_bus_activity == 0) {
        controller->last_bus_activity = current_time;
    }
    
    // Update bias state based on CAN transceiver state
    switch (can_state) {
        case CAN_STATE_OFF:
            controller->state = BIAS_STATE_OFF;
            break;
            
        case CAN_STATE_AUTONOMOUS_INACTIVE:
            controller->state = BIAS_STATE_AUTONOMOUS_INACTIVE;
            break;
            
        case CAN_STATE_AUTONOMOUS_ACTIVE:
            controller->state = BIAS_STATE_AUTONOMOUS_ACTIVE;
            break;
            
        case CAN_STATE_ACTIVE:
            controller->state = BIAS_STATE_ACTIVE;
            break;
    }
}

void bus_bias_controller_get_bias(
    const BusBiasController* controller,
    double vcc,
    double* canh,
    double* canl
) {
    if (!controller || !canh || !canl) return;
    
    // Set bias voltages based on state
    switch (controller->state) {
        case BIAS_STATE_OFF:
            // High impedance - no bias voltage (represented as 0)
            *canh = 0.0;
            *canl = 0.0;
            break;
            
        case BIAS_STATE_AUTONOMOUS_INACTIVE:
            // Bias to GND
            *canh = BIAS_VOLTAGE_GND;
            *canl = BIAS_VOLTAGE_GND;
            break;
            
        case BIAS_STATE_AUTONOMOUS_ACTIVE:
            // Bias to 2.5V
            *canh = BIAS_VOLTAGE_2_5V;
            *canl = BIAS_VOLTAGE_2_5V;
            break;
            
        case BIAS_STATE_ACTIVE:
            // Bias to VCC/2
            *canh = vcc / 2.0;
            *canl = vcc / 2.0;
            break;
    }
}

bool bus_bias_controller_is_silence_timeout(
    const BusBiasController* controller,
    uint64_t current_time
) {
    if (!controller) return false;
    
    uint64_t silence_duration = current_time - controller->last_bus_activity;
    return silence_duration > TSILENCE_NS;
}
