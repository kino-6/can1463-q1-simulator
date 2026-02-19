#include "can_transceiver.h"
#include <string.h>
#include <stdio.h>

// Voltage thresholds for bus state detection
#define VDIFF_DOMINANT_THRESHOLD 0.9   // >= 0.9V is dominant
#define VDIFF_RECESSIVE_THRESHOLD 0.5  // <= 0.5V is recessive

// Bus voltage levels
#define CANH_DOMINANT_VOLTAGE 3.5
#define CANL_DOMINANT_VOLTAGE 1.5
#define CANH_RECESSIVE_VOLTAGE 2.5
#define CANL_RECESSIVE_VOLTAGE 2.5

// Silence timeout for autonomous state transition (in nanoseconds)
#define TSILENCE_NS (1000000000ULL)  // 1 second (middle of 0.6-1.2s range)

// Last bus activity timestamp (static for state machine)
static uint64_t last_bus_activity_time = 0;

void can_transceiver_init(CANTransceiver* transceiver) {
    if (!transceiver) return;
    
    memset(transceiver, 0, sizeof(CANTransceiver));
    transceiver->state = CAN_STATE_OFF;
    transceiver->driver_enabled = false;
    transceiver->receiver_enabled = false;
    transceiver->canh_voltage = 0.0;
    transceiver->canl_voltage = 0.0;
    transceiver->rxd_output = true;  // Default high (recessive)
    transceiver->rxd_pending = false;
    transceiver->rxd_pending_value = true;
    transceiver->rxd_update_time = 0;
    last_bus_activity_time = 0;
}

BusState can_transceiver_get_bus_state(double vdiff) {
    if (vdiff >= VDIFF_DOMINANT_THRESHOLD) {
        return BUS_STATE_DOMINANT;
    } else if (vdiff <= VDIFF_RECESSIVE_THRESHOLD) {
        return BUS_STATE_RECESSIVE;
    } else {
        return BUS_STATE_INDETERMINATE;
    }
}

void can_transceiver_drive_bus(
    CANTransceiver* transceiver,
    bool dominant,
    double* canh,
    double* canl
) {
    if (!transceiver || !canh || !canl) return;
    
    // Only drive if driver is enabled
    if (dominant && transceiver->driver_enabled) {
        // Drive dominant: CANH high, CANL low
        *canh = CANH_DOMINANT_VOLTAGE;
        *canl = CANL_DOMINANT_VOLTAGE;
        transceiver->canh_voltage = *canh;
        transceiver->canl_voltage = *canl;
    } else {
        // Recessive or driver disabled: high impedance (bus bias takes over)
        // Set to recessive bias voltage
        *canh = CANH_RECESSIVE_VOLTAGE;
        *canl = CANL_RECESSIVE_VOLTAGE;
        transceiver->canh_voltage = *canh;
        transceiver->canl_voltage = *canl;
    }
}

void can_transceiver_update_rxd(
    CANTransceiver* transceiver,
    BusState bus_state,
    uint64_t current_time,
    uint64_t schedule_time
) {
    if (!transceiver) return;
    
    if (!transceiver->receiver_enabled) {
        transceiver->rxd_output = true;  // High when receiver disabled
        transceiver->rxd_pending = false;
        return;
    }
    
    // First, check if pending RXD update should be applied (using current_time)
    if (transceiver->rxd_pending && current_time >= transceiver->rxd_update_time) {
        transceiver->rxd_output = transceiver->rxd_pending_value;
        transceiver->rxd_pending = false;
    }
    
    // Determine target RXD value based on bus state
    bool target_rxd;
    switch (bus_state) {
        case BUS_STATE_DOMINANT:
            target_rxd = false;  // RXD low for dominant
            break;
        case BUS_STATE_RECESSIVE:
            target_rxd = true;   // RXD high for recessive
            break;
        case BUS_STATE_INDETERMINATE:
            // Keep previous state for indeterminate
            return;
    }
    
    // Check if RXD needs to change
    // Only schedule a new update if:
    // 1. Target is different from current output
    // 2. No pending update is already scheduled OR the pending update is for a different value
    bool needs_update = (target_rxd != transceiver->rxd_output);
    bool pending_is_stale = transceiver->rxd_pending && (transceiver->rxd_pending_value != target_rxd);
    
    if (needs_update && (!transceiver->rxd_pending || pending_is_stale)) {
        // Determine propagation delay based on transition type
        uint64_t prop_delay;
        if (!target_rxd) {
            // Recessive-to-dominant transition (RXD going low)
            // Use middle of TPROP_LOOP1 range (100-190ns)
            prop_delay = (TPROP_LOOP1_MIN_NS + TPROP_LOOP1_MAX_NS) / 2;
        } else {
            // Dominant-to-recessive transition (RXD going high)
            // Use middle of TPROP_LOOP2 range (110-190ns)
            prop_delay = (TPROP_LOOP2_MIN_NS + TPROP_LOOP2_MAX_NS) / 2;
        }
        
        // Calculate when the update should occur
        uint64_t update_time = schedule_time + prop_delay;
        
        // If the update time is in the past or now, apply it immediately
        if (update_time <= current_time) {
            transceiver->rxd_output = target_rxd;
            transceiver->rxd_pending = false;
        } else {
            // Schedule RXD update for the future
            transceiver->rxd_pending = true;
            transceiver->rxd_pending_value = target_rxd;
            transceiver->rxd_update_time = update_time;
        }
    }
}

void can_transceiver_update_state_machine(
    CANTransceiver* transceiver,
    OperatingMode mode,
    BusState bus_state,
    bool vsup_valid,
    uint64_t current_time
) {
    if (!transceiver) return;
    
    CANTransceiverState prev_state = transceiver->state;
    
    // Track bus activity for silence timeout
    if (bus_state == BUS_STATE_DOMINANT) {
        last_bus_activity_time = current_time;
    }
    
    // Initialize last_bus_activity_time if it's zero
    if (last_bus_activity_time == 0) {
        last_bus_activity_time = current_time;
    }
    
    // State machine transitions
    switch (transceiver->state) {
        case CAN_STATE_OFF:
            if (vsup_valid) {
                transceiver->state = CAN_STATE_AUTONOMOUS_INACTIVE;
            }
            break;
            
        case CAN_STATE_AUTONOMOUS_INACTIVE:
            if (!vsup_valid) {
                transceiver->state = CAN_STATE_OFF;
            } else if (mode == MODE_NORMAL || mode == MODE_SILENT) {
                transceiver->state = CAN_STATE_ACTIVE;
            } else if (bus_state == BUS_STATE_DOMINANT) {
                // Remote wake-up detected
                transceiver->state = CAN_STATE_AUTONOMOUS_ACTIVE;
                last_bus_activity_time = current_time;
            }
            break;
            
        case CAN_STATE_AUTONOMOUS_ACTIVE:
            if (!vsup_valid) {
                transceiver->state = CAN_STATE_OFF;
            } else if (mode == MODE_NORMAL || mode == MODE_SILENT) {
                transceiver->state = CAN_STATE_ACTIVE;
            } else {
                // Check for silence timeout
                uint64_t silence_duration = current_time - last_bus_activity_time;
                if (silence_duration > TSILENCE_NS) {
                    transceiver->state = CAN_STATE_AUTONOMOUS_INACTIVE;
                }
            }
            break;
            
        case CAN_STATE_ACTIVE:
            if (!vsup_valid) {
                transceiver->state = CAN_STATE_OFF;
            } else if (mode != MODE_NORMAL && mode != MODE_SILENT) {
                // Exiting active mode
                if (bus_state == BUS_STATE_DOMINANT || 
                    (current_time - last_bus_activity_time) <= TSILENCE_NS) {
                    transceiver->state = CAN_STATE_AUTONOMOUS_ACTIVE;
                } else {
                    transceiver->state = CAN_STATE_AUTONOMOUS_INACTIVE;
                }
            }
            break;
    }
    
    // Update driver and receiver enable based on state and mode
    switch (transceiver->state) {
        case CAN_STATE_OFF:
            transceiver->driver_enabled = false;
            transceiver->receiver_enabled = false;
            break;
            
        case CAN_STATE_AUTONOMOUS_INACTIVE:
        case CAN_STATE_AUTONOMOUS_ACTIVE:
            transceiver->driver_enabled = false;
            transceiver->receiver_enabled = true;
            break;
            
        case CAN_STATE_ACTIVE:
            if (mode == MODE_NORMAL) {
                transceiver->driver_enabled = true;
                transceiver->receiver_enabled = true;
            } else if (mode == MODE_SILENT) {
                transceiver->driver_enabled = false;
                transceiver->receiver_enabled = true;
            } else {
                transceiver->driver_enabled = false;
                transceiver->receiver_enabled = false;
            }
            break;
    }
}

void can_transceiver_update(
    CANTransceiver* transceiver,
    OperatingMode mode,
    bool txd_low,
    double canh_voltage,
    double canl_voltage,
    uint64_t current_time
) {
    if (!transceiver) return;
    
    // Calculate differential voltage from input bus
    double vdiff = canh_voltage - canl_voltage;
    
    // Get bus state from differential voltage
    BusState bus_state = can_transceiver_get_bus_state(vdiff);
    
    // Update state machine (determines driver/receiver enable)
    bool vsup_valid = (mode != MODE_OFF);
    can_transceiver_update_state_machine(
        transceiver,
        mode,
        bus_state,
        vsup_valid,
        current_time
    );
    
    // Drive bus based on TXD input and driver enable
    double canh_out, canl_out;
    can_transceiver_drive_bus(transceiver, txd_low, &canh_out, &canl_out);
    
    // Note: RXD update is handled separately in simulator_step after bus is driven
}
