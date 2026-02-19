#include "power_monitor.h"
#include <string.h>

// Convert milliseconds to nanoseconds
#define MS_TO_NS(ms) ((uint64_t)(ms) * 1000000ULL)

// Helper function to check if voltage is in falling range
static bool is_in_falling_range(double voltage, double min, double max) {
    return voltage >= min && voltage <= max;
}

// Helper function to check if voltage is below falling threshold
static bool is_below_falling(double voltage, double max) {
    return voltage < max;
}

// Helper function to check if voltage is above rising threshold
static bool is_above_rising(double voltage, double min) {
    return voltage > min;
}

void power_monitor_init(PowerState* state) {
    if (!state) return;
    
    memset(state, 0, sizeof(PowerState));
    
    // Initialize with nominal voltages
    state->vsup = 12.0;
    state->vcc = 5.0;
    state->vio = 3.3;
    
    // All flags start cleared
    state->uvsup_flag = false;
    state->uvcc_flag = false;
    state->uvio_flag = false;
    state->pwron_flag = false;
    
    // Initialize timing with sentinel value (not started)
    state->uvcc_start_time = UINT64_MAX;
    state->uvio_start_time = UINT64_MAX;
}

void power_monitor_update(PowerState* state, double vsup, double vcc,
                         double vio, uint64_t current_time) {
    if (!state) return;
    
    // Store previous voltages for comparison
    double prev_vcc = state->vcc;
    double prev_vio = state->vio;
    
    // Store current voltages
    state->vsup = vsup;
    state->vcc = vcc;
    state->vio = vio;
    
    // --- VSUP Monitoring ---
    // VSUP has no filter time - immediate response
    // Requirement 4.1: VSUP < UVSUP(F) sets flag and enters Off mode
    // Requirement 4.4: VSUP > UVSUP(R) clears flag and sets PWRON
    
    // Hysteresis logic:
    // - Falling threshold: UVSUP_FALLING_MIN (3.5V)
    // - Rising threshold: UVSUP_RISING_MIN (3.85V)
    // - If flag is NOT set and voltage drops below falling threshold → set flag
    // - If flag IS set and voltage rises above rising threshold → clear flag and set PWRON
    // - Otherwise → maintain current state
    
    if (!state->uvsup_flag && vsup <= UVSUP_FALLING_MIN) {
        // Voltage dropped below or at falling threshold → set flag
        state->uvsup_flag = true;
    } else if (state->uvsup_flag && vsup > UVSUP_RISING_MIN) {
        // Voltage rose above rising threshold → clear flag and set PWRON
        state->uvsup_flag = false;
        state->pwron_flag = true;
    }
    
    // --- VCC Monitoring ---
    // Requirement 4.2: VCC < UVCC(F) for t >= tUV sets flag
    if (vcc < UVCC_FALLING_MAX) {
        // Start timing if not already started
        if (state->uvcc_start_time == UINT64_MAX) {
            state->uvcc_start_time = current_time;
        }
        
        // Check if filter time has elapsed
        uint64_t elapsed_ns = current_time - state->uvcc_start_time;
        uint64_t tuv_min_ns = MS_TO_NS(TUV_MIN_MS);
        
        if (elapsed_ns >= tuv_min_ns && !state->uvcc_flag) {
            state->uvcc_flag = true;
        }
    }
    // Requirement 4.5: VCC > UVCC(R) clears flag
    else if (vcc > UVCC_RISING_MIN) {
        if (state->uvcc_flag) {
            state->uvcc_flag = false;
        }
        // Reset timing
        state->uvcc_start_time = UINT64_MAX;
    }
    // Voltage is in hysteresis band - maintain current state but reset timer if rising
    else {
        if (vcc > prev_vcc) {  // Voltage is rising
            state->uvcc_start_time = UINT64_MAX;
        }
    }
    
    // --- VIO Monitoring ---
    // Requirement 4.3: VIO < UVIO(F) for t >= tUV sets flag
    if (vio < UVIO_FALLING_MAX) {
        // Start timing if not already started (and current_time is not 0)
        if (state->uvio_start_time == UINT64_MAX) {
            state->uvio_start_time = current_time;
        }
        
        // Check if filter time has elapsed
        uint64_t elapsed_ns = current_time - state->uvio_start_time;
        uint64_t tuv_min_ns = MS_TO_NS(TUV_MIN_MS);
        
        if (elapsed_ns >= tuv_min_ns && !state->uvio_flag) {
            state->uvio_flag = true;
        }
    }
    // Requirement 4.6: VIO > UVIO(R) clears flag
    else if (vio > UVIO_RISING_MIN) {
        if (state->uvio_flag) {
            state->uvio_flag = false;
        }
        // Reset timing
        state->uvio_start_time = UINT64_MAX;
    }
    // Voltage is in hysteresis band - maintain current state but reset timer if rising
    else {
        if (vio > prev_vio) {  // Voltage is rising
            state->uvio_start_time = UINT64_MAX;
        }
    }
}

bool power_monitor_is_vsup_valid(const PowerState* state) {
    if (!state) return false;
    return !state->uvsup_flag;
}

bool power_monitor_is_vcc_valid(const PowerState* state) {
    if (!state) return false;
    return !state->uvcc_flag;
}

bool power_monitor_is_vio_valid(const PowerState* state) {
    if (!state) return false;
    return !state->uvio_flag;
}

bool power_monitor_get_uvsup_flag(const PowerState* state) {
    if (!state) return false;
    return state->uvsup_flag;
}

bool power_monitor_get_uvcc_flag(const PowerState* state) {
    if (!state) return false;
    return state->uvcc_flag;
}

bool power_monitor_get_uvio_flag(const PowerState* state) {
    if (!state) return false;
    return state->uvio_flag;
}

bool power_monitor_get_pwron_flag(const PowerState* state) {
    if (!state) return false;
    return state->pwron_flag;
}

void power_monitor_clear_pwron_flag(PowerState* state) {
    if (!state) return;
    state->pwron_flag = false;
}
