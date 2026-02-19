#include "wake_handler.h"
#include <string.h>

// Convert microseconds to nanoseconds
#define US_TO_NS(us) ((uint64_t)((us) * 1000.0))

// Convert milliseconds to nanoseconds
#define MS_TO_NS(ms) ((uint64_t)((ms) * 1000000.0))

// Convert seconds to nanoseconds
#define S_TO_NS(s) ((uint64_t)((s) * 1000000000.0))

void wake_handler_init(WakeState* state) {
    if (!state) return;
    
    memset(state, 0, sizeof(WakeState));
    
    // Initialize to idle state
    state->wakerq_flag = false;
    state->wakesr_flag = false;
    state->wake_source_local = false;
    state->wup_state = WUP_STATE_IDLE;
    state->wup_phase_start = UINT64_MAX;  // Use sentinel value for "not started"
    state->wup_timeout_start = UINT64_MAX;  // Use sentinel value for "not started"
    state->wake_pin_prev_state = false;
}

void wake_handler_update(WakeState* state, BusState bus_state,
                        bool wake_pin_high, OperatingMode mode,
                        uint64_t current_time) {
    if (!state) return;
    
    // Only process wake-up events in Standby or Sleep mode
    // Requirement 6.1: Valid WUP in Standby or Sleep mode sets WAKERQ
    // Requirement 6.2: WAKE pin transition in Sleep mode sets WAKERQ
    if (mode == MODE_STANDBY || mode == MODE_SLEEP) {
        // Process remote wake-up (WUP pattern on CAN bus)
        wake_handler_process_wup(state, bus_state, current_time);
        
        // Process local wake-up (WAKE pin transition)
        // Only in Sleep mode per requirement 6.2
        if (mode == MODE_SLEEP) {
            wake_handler_process_lwu(state, wake_pin_high, current_time);
        }
    } else {
        // Not in wake-up capable mode, reset WUP state machine
        if (state->wup_state != WUP_STATE_IDLE) {
            state->wup_state = WUP_STATE_IDLE;
            state->wup_phase_start = UINT64_MAX;
            state->wup_timeout_start = UINT64_MAX;
        }
    }
    
    // Update previous WAKE pin state for edge detection
    // This must happen AFTER processing LWU to ensure edge detection works correctly
    state->wake_pin_prev_state = wake_pin_high;
}

void wake_handler_process_wup(WakeState* state, BusState bus_state,
                              uint64_t current_time) {
    if (!state) return;
    
    // WUP pattern: filtered dominant, filtered recessive, filtered dominant
    // Each phase must be >= tWK_FILTER (0.5-1.8μs)
    // Total pattern must complete within tWK_TIMEOUT (0.8-2ms)
    
    // Use minimum filter time for detection
    uint64_t filter_time_ns = US_TO_NS(TWK_FILTER_MIN_US);
    uint64_t timeout_ns = MS_TO_NS(TWK_TIMEOUT_MAX_MS);
    
    // Check for timeout (only if timeout timer is running)
    if (state->wup_timeout_start != UINT64_MAX && 
        state->wup_state != WUP_STATE_IDLE && 
        state->wup_state != WUP_STATE_COMPLETE) {
        uint64_t elapsed = current_time - state->wup_timeout_start;
        if (elapsed >= timeout_ns) {
            // Timeout expired, reset state machine
            // Requirement 6.6: WUP timeout resets detection
            state->wup_state = WUP_STATE_IDLE;
            state->wup_phase_start = UINT64_MAX;
            state->wup_timeout_start = UINT64_MAX;
            return;  // Exit early after timeout
        }
    }
    
    switch (state->wup_state) {
        case WUP_STATE_IDLE:
            // Waiting for first dominant
            if (bus_state == BUS_STATE_DOMINANT) {
                state->wup_state = WUP_STATE_FIRST_DOMINANT;
                state->wup_phase_start = current_time;
                state->wup_timeout_start = current_time;
            }
            break;
            
        case WUP_STATE_FIRST_DOMINANT:
            if (bus_state == BUS_STATE_DOMINANT) {
                // Still dominant, check if filter time met
                uint64_t elapsed = current_time - state->wup_phase_start;
                if (elapsed >= filter_time_ns) {
                    // First dominant phase complete, wait for recessive
                    state->wup_state = WUP_STATE_RECESSIVE;
                    state->wup_phase_start = current_time;
                }
            } else {
                // Bus went recessive too early, reset
                state->wup_state = WUP_STATE_IDLE;
                state->wup_phase_start = UINT64_MAX;
                state->wup_timeout_start = UINT64_MAX;
            }
            break;
            
        case WUP_STATE_RECESSIVE:
            if (bus_state == BUS_STATE_RECESSIVE) {
                // Check if filter time met
                uint64_t elapsed = current_time - state->wup_phase_start;
                if (elapsed >= filter_time_ns) {
                    // Recessive phase complete, wait for second dominant
                    state->wup_state = WUP_STATE_SECOND_DOMINANT;
                    state->wup_phase_start = current_time;
                }
            } else if (bus_state == BUS_STATE_DOMINANT) {
                // Bus went dominant too early, could be start of second dominant
                // Check if we had enough recessive time
                uint64_t elapsed = current_time - state->wup_phase_start;
                if (elapsed >= filter_time_ns) {
                    // Recessive was long enough, move to second dominant
                    state->wup_state = WUP_STATE_SECOND_DOMINANT;
                    state->wup_phase_start = current_time;
                } else {
                    // Not enough recessive time, reset
                    state->wup_state = WUP_STATE_IDLE;
                    state->wup_phase_start = UINT64_MAX;
                    state->wup_timeout_start = UINT64_MAX;
                }
            }
            break;
            
        case WUP_STATE_SECOND_DOMINANT:
            if (bus_state == BUS_STATE_DOMINANT) {
                // Check if filter time met
                uint64_t elapsed = current_time - state->wup_phase_start;
                if (elapsed >= filter_time_ns) {
                    // WUP pattern complete!
                    // Requirement 6.1: Set WAKERQ flag
                    state->wakerq_flag = true;
                    state->wakesr_flag = true;
                    state->wake_source_local = false;  // Remote wake-up
                    state->wup_state = WUP_STATE_COMPLETE;
                    state->wup_phase_start = UINT64_MAX;
                    state->wup_timeout_start = UINT64_MAX;
                }
            } else {
                // Bus went recessive too early, reset
                state->wup_state = WUP_STATE_IDLE;
                state->wup_phase_start = UINT64_MAX;
                state->wup_timeout_start = UINT64_MAX;
            }
            break;
            
        case WUP_STATE_COMPLETE:
            // Pattern already complete, stay in this state until mode changes
            // or flags are cleared
            break;
    }
}

void wake_handler_process_lwu(WakeState* state, bool wake_pin_high,
                              uint64_t current_time) {
    if (!state) return;
    
    // Requirement 6.2: WAKE pin transition (rising or falling edge) sets WAKERQ
    // Detect edge by comparing with previous state
    bool edge_detected = (wake_pin_high != state->wake_pin_prev_state);
    
    if (edge_detected) {
        // Local wake-up event detected
        state->wakerq_flag = true;
        state->wakesr_flag = true;
        state->wake_source_local = true;  // Local wake-up
        
        // Reset WUP state machine since we have a wake event
        state->wup_state = WUP_STATE_IDLE;
        state->wup_phase_start = UINT64_MAX;
        state->wup_timeout_start = UINT64_MAX;
    }
}

void wake_handler_clear_flags(WakeState* state) {
    if (!state) return;
    
    // Requirement 6.4: Clear WAKERQ when transitioning to Normal mode
    state->wakerq_flag = false;
    // Note: WAKESR is not cleared here - it persists to indicate wake source
    
    // Reset WUP state machine
    state->wup_state = WUP_STATE_IDLE;
    state->wup_phase_start = UINT64_MAX;
    state->wup_timeout_start = UINT64_MAX;
}

bool wake_handler_get_wakerq(const WakeState* state) {
    if (!state) return false;
    return state->wakerq_flag;
}

bool wake_handler_get_wakesr(const WakeState* state) {
    if (!state) return false;
    return state->wakesr_flag;
}

bool wake_handler_is_local_wake(const WakeState* state) {
    if (!state) return false;
    return state->wake_source_local;
}
