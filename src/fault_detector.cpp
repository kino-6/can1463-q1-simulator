#include "fault_detector.h"
#include <string.h>

// Convert milliseconds to nanoseconds
#define MS_TO_NS(ms) ((uint64_t)(ms * 1000000.0))

// Convert microseconds to nanoseconds
#define US_TO_NS(us) ((uint64_t)(us * 1000.0))

void fault_detector_init(FaultState* state) {
    if (!state) return;
    
    memset(state, 0, sizeof(FaultState));
    state->txd_dominant_start = UINT64_MAX;
    state->bus_dominant_start = UINT64_MAX;
}

void fault_detector_check_txdclp(
    FaultState* state,
    bool txd_low,
    OperatingMode entering_mode
) {
    if (!state) return;
    
    // Requirement 5.1: WHEN TXD is low before entering Normal mode,
    // THE Fault_Detector SHALL set TXDCLP flag and disable CAN driver
    if (entering_mode == MODE_NORMAL && txd_low) {
        state->txdclp_flag = true;
    }
}

void fault_detector_check_txddto(
    FaultState* state,
    bool txd_low,
    uint64_t current_time
) {
    if (!state) return;
    
    // Requirement 5.2: WHEN TXD is dominant for t >= tTXDDTO (1.2-3.8ms),
    // THE Fault_Detector SHALL set TXDDTO flag and disable CAN driver
    
    if (txd_low) {
        // TXD is dominant
        if (state->txd_dominant_start == UINT64_MAX) {
            // Start tracking dominant time
            state->txd_dominant_start = current_time;
        } else {
            // Check if timeout exceeded
            uint64_t dominant_duration = current_time - state->txd_dominant_start;
            if (dominant_duration >= MS_TO_NS(TTXDDTO_MIN_MS)) {
                state->txddto_flag = true;
            }
        }
    } else {
        // TXD is recessive, reset tracking
        state->txd_dominant_start = UINT64_MAX;
    }
}

void fault_detector_check_txdrxd(
    FaultState* state,
    bool txd_low,
    bool rxd_low,
    uint64_t current_time
) {
    if (!state) return;
    
    // Requirement 5.3: WHEN TXD and RXD are shorted for t >= tTXDDTO,
    // THE Fault_Detector SHALL set TXDRXD flag and disable CAN driver
    
    // TXD and RXD are shorted if they have the same value
    bool shorted = (txd_low == rxd_low);
    
    if (shorted) {
        if (state->txd_dominant_start == UINT64_MAX) {
            state->txd_dominant_start = current_time;
        } else {
            uint64_t short_duration = current_time - state->txd_dominant_start;
            if (short_duration >= MS_TO_NS(TTXDDTO_MIN_MS)) {
                state->txdrxd_flag = true;
            }
        }
    } else {
        // Not shorted, reset tracking
        state->txd_dominant_start = UINT64_MAX;
    }
}

void fault_detector_check_candom(
    FaultState* state,
    BusState bus_state,
    uint64_t current_time
) {
    if (!state) return;
    
    // Requirement 5.4: WHEN CAN bus is dominant for t >= tBUSDOM (1.4-3.8ms),
    // THE Fault_Detector SHALL set CANDOM flag
    
    if (bus_state == BUS_STATE_DOMINANT) {
        if (state->bus_dominant_start == UINT64_MAX) {
            state->bus_dominant_start = current_time;
        } else {
            uint64_t dominant_duration = current_time - state->bus_dominant_start;
            if (dominant_duration >= MS_TO_NS(TBUSDOM_MIN_MS)) {
                state->candom_flag = true;
            }
        }
    } else {
        // Bus is recessive, reset tracking
        state->bus_dominant_start = UINT64_MAX;
    }
}

void fault_detector_check_tsd(
    FaultState* state,
    double tj_temperature
) {
    if (!state) return;
    
    // Requirement 5.5: WHEN junction temperature TJ >= TSDR (165°C),
    // THE Fault_Detector SHALL set TSD flag and disable CAN driver
    
    if (tj_temperature >= TSDR_CELSIUS) {
        state->tsd_flag = true;
    } else {
        // Clear TSD flag when temperature drops below threshold
        state->tsd_flag = false;
    }
}

void fault_detector_check_cbf(
    FaultState* state,
    BusState bus_state,
    OperatingMode mode
) {
    if (!state) return;
    
    // Requirement 5.6: WHEN CAN bus fault persists for four consecutive
    // dominant-to-recessive transitions, THE Fault_Detector SHALL set CBF flag
    
    // Only track in active modes
    if (mode != MODE_NORMAL && mode != MODE_SILENT) {
        state->cbf_transition_count = 0;
        return;
    }
    
    // Track dominant-to-recessive transitions
    static BusState prev_bus_state = BUS_STATE_RECESSIVE;
    
    if (prev_bus_state == BUS_STATE_DOMINANT && bus_state == BUS_STATE_RECESSIVE) {
        state->cbf_transition_count++;
        
        if (state->cbf_transition_count >= 4) {
            state->cbf_flag = true;
        }
    }
    
    prev_bus_state = bus_state;
}

void fault_detector_update(
    FaultState* state,
    bool txd_low,
    bool rxd_low,
    BusState bus_state,
    double tj_temperature,
    uint64_t current_time,
    OperatingMode mode
) {
    if (!state) return;
    
    // Check all fault conditions
    fault_detector_check_txddto(state, txd_low, current_time);
    fault_detector_check_txdrxd(state, txd_low, rxd_low, current_time);
    fault_detector_check_candom(state, bus_state, current_time);
    fault_detector_check_tsd(state, tj_temperature);
    fault_detector_check_cbf(state, bus_state, mode);
}

bool fault_detector_has_any_fault(const FaultState* state) {
    if (!state) return false;
    
    return state->txdclp_flag ||
           state->txddto_flag ||
           state->txdrxd_flag ||
           state->candom_flag ||
           state->tsd_flag ||
           state->cbf_flag;
}

bool fault_detector_get_nfault_state(const FaultState* state) {
    // Requirement 5.7: WHEN a fault is detected,
    // THE Fault_Detector SHALL drive nFAULT pin low
    
    // nFAULT is active low, so return true if any fault is present
    return fault_detector_has_any_fault(state);
}

bool fault_detector_should_disable_driver(const FaultState* state) {
    if (!state) return false;
    
    // Driver should be disabled for these faults:
    // - TXDCLP (TXD clamped low)
    // - TXDDTO (TXD dominant timeout)
    // - TXDRXD (TXD/RXD short)
    // - TSD (Thermal shutdown)
    // Note: CANDOM and CBF do NOT disable the driver
    
    return state->txdclp_flag ||
           state->txddto_flag ||
           state->txdrxd_flag ||
           state->tsd_flag;
}
