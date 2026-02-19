#include "inh_controller.h"
#include <string.h>

// INH timing parameter (in nanoseconds)
#define TINH_SLP_STB_NS (TINH_SLP_STB_US * 1000ULL)  // 100μs

// INH output voltage when high (VSUP - 0.75V, middle of 0.5V-1V range)
#define INH_OUTPUT_VOLTAGE_DROP 0.75

void inh_controller_init(INHController* controller) {
    if (!controller) return;
    
    memset(controller, 0, sizeof(INHController));
    controller->inh_enabled = true;  // Default enabled (INH_MASK low/floating)
    controller->inh_output_high = false;
    controller->wake_event_time = 0;
    controller->pending_inh_assertion = false;
}

void inh_controller_update(
    INHController* controller,
    OperatingMode mode,
    bool inh_mask_high,
    bool wake_event,
    uint64_t current_time
) {
    if (!controller) return;
    
    // Update INH enable based on INH_MASK pin
    // INH is disabled when INH_MASK is high, enabled when low or floating
    controller->inh_enabled = !inh_mask_high;
    
    // If INH is disabled by mask, set output to high impedance
    if (!controller->inh_enabled) {
        controller->inh_output_high = false;
        controller->pending_inh_assertion = false;
        return;
    }
    
    // Handle wake-up event timing
    if (wake_event) {
        controller->wake_event_time = current_time;
        controller->pending_inh_assertion = true;
    }
    
    // Check if INH assertion delay has elapsed after wake-up
    if (controller->pending_inh_assertion) {
        uint64_t time_since_wake = current_time - controller->wake_event_time;
        if (time_since_wake >= TINH_SLP_STB_NS) {
            controller->pending_inh_assertion = false;
        }
    }
    
    // Determine INH output state based on operating mode
    bool should_be_high = false;
    
    switch (mode) {
        case MODE_NORMAL:
        case MODE_SILENT:
        case MODE_STANDBY:
            // INH should be high in these modes
            should_be_high = true;
            break;
            
        case MODE_SLEEP:
        case MODE_GO_TO_SLEEP:
        case MODE_OFF:
            // INH should be high impedance in these modes
            should_be_high = false;
            break;
    }
    
    // Apply wake-up timing constraint
    // If we're transitioning from Sleep to Standby, wait for tINH_SLP_STB
    if (should_be_high && controller->pending_inh_assertion) {
        // Don't assert INH yet, wait for timing delay
        controller->inh_output_high = false;
    } else {
        controller->inh_output_high = should_be_high;
    }
}

void inh_controller_get_pin_state(
    const INHController* controller,
    PinState* state,
    double* voltage
) {
    if (!controller || !state || !voltage) return;
    
    if (!controller->inh_enabled || !controller->inh_output_high) {
        // INH disabled or output low -> high impedance
        *state = PIN_STATE_HIGH_IMPEDANCE;
        *voltage = 0.0;
    } else {
        // INH enabled and output high -> drive high
        *state = PIN_STATE_HIGH;
        // Voltage is VSUP - 0.5V to 1V (we use middle value)
        // Assuming VSUP is typically 5V, output would be ~4.25V
        // But we need VSUP value from power monitor
        // For now, use a typical value
        *voltage = 5.0 - INH_OUTPUT_VOLTAGE_DROP;  // ~4.25V
    }
}
