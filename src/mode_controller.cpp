#include "mode_controller.h"
#include <string.h>

// Convert seconds to nanoseconds
#define S_TO_NS(s) ((uint64_t)(s * 1000000000ULL))

// tSILENCE timeout range (0.6-1.2s) - using minimum for conservative behavior
#define TSILENCE_NS S_TO_NS(TSILENCE_MIN_S)

/**
 * Mode transition table
 * Defines valid transitions between operating modes
 */
typedef struct {
    OperatingMode from;
    OperatingMode to;
} ModeTransition;

// Valid mode transitions based on design document
static const ModeTransition valid_transitions[] = {
    // From Off
    {MODE_OFF, MODE_NORMAL},
    {MODE_OFF, MODE_SILENT},
    
    // Between Normal and Silent
    {MODE_NORMAL, MODE_SILENT},
    {MODE_SILENT, MODE_NORMAL},
    
    // From Normal/Silent to Standby
    {MODE_NORMAL, MODE_STANDBY},
    {MODE_SILENT, MODE_STANDBY},
    
    // From Normal/Silent to Go-to-sleep
    {MODE_NORMAL, MODE_GO_TO_SLEEP},
    {MODE_SILENT, MODE_GO_TO_SLEEP},
    
    // From Go-to-sleep to Sleep
    {MODE_GO_TO_SLEEP, MODE_SLEEP},
    
    // From Standby to Normal/Silent
    {MODE_STANDBY, MODE_NORMAL},
    {MODE_STANDBY, MODE_SILENT},
    
    // From Sleep to Standby (wake-up)
    {MODE_SLEEP, MODE_STANDBY},
    
    // Any mode to Off (power loss)
    {MODE_NORMAL, MODE_OFF},
    {MODE_SILENT, MODE_OFF},
    {MODE_STANDBY, MODE_OFF},
    {MODE_GO_TO_SLEEP, MODE_OFF},
    {MODE_SLEEP, MODE_OFF},
};

static const int num_valid_transitions = sizeof(valid_transitions) / sizeof(ModeTransition);

void mode_controller_init(ModeState* state) {
    if (!state) return;
    
    memset(state, 0, sizeof(ModeState));
    
    // Start in Off mode
    state->current_mode = MODE_OFF;
    state->previous_mode = MODE_OFF;
    state->mode_entry_time = 0;
    state->wakerq_flag = false;
}

bool mode_controller_can_transition(OperatingMode from, OperatingMode to) {
    // Same mode is always valid (no transition)
    if (from == to) {
        return true;
    }
    
    // Check transition table
    for (int i = 0; i < num_valid_transitions; i++) {
        if (valid_transitions[i].from == from && valid_transitions[i].to == to) {
            return true;
        }
    }
    
    return false;
}

/**
 * Determine target mode based on inputs
 * This implements the mode transition logic from the design document
 */
static OperatingMode determine_target_mode(
    OperatingMode current_mode,
    bool en_high,
    bool nstb_high,
    bool vsup_valid,
    bool wakerq_set,
    uint64_t time_in_mode
) {
    // Priority 1: Power loss - always go to Off mode
    if (!vsup_valid) {
        return MODE_OFF;
    }
    
    // Priority 2: Check for automatic transitions
    // Go-to-sleep → Sleep after tSILENCE timeout
    if (current_mode == MODE_GO_TO_SLEEP && time_in_mode >= TSILENCE_NS) {
        return MODE_SLEEP;
    }
    
    // Priority 3: Pin-based mode determination
    if (nstb_high) {
        // nSTB is high → Normal or Silent mode
        if (en_high) {
            return MODE_NORMAL;
        } else {
            return MODE_SILENT;
        }
    } else {
        // nSTB is low → Standby, Go-to-sleep, or Sleep
        if (wakerq_set) {
            return MODE_STANDBY;
        } else {
            // WAKERQ is cleared
            // If currently in Sleep, stay in Sleep
            if (current_mode == MODE_SLEEP) {
                return MODE_SLEEP;
            }
            // Otherwise, go to Go-to-sleep (transitional state)
            return MODE_GO_TO_SLEEP;
        }
    }
}

OperatingMode mode_controller_update(
    ModeState* state,
    bool en_high,
    bool nstb_high,
    bool vsup_valid,
    bool wakerq_set,
    uint64_t current_time
) {
    if (!state) return MODE_OFF;
    
    // Calculate time in current mode
    uint64_t time_in_mode = 0;
    if (current_time >= state->mode_entry_time) {
        time_in_mode = current_time - state->mode_entry_time;
    }
    
    // Determine target mode based on inputs
    OperatingMode target_mode = determine_target_mode(
        state->current_mode,
        en_high,
        nstb_high,
        vsup_valid,
        wakerq_set,
        time_in_mode
    );
    
    // Check if transition is valid
    if (target_mode != state->current_mode) {
        if (mode_controller_can_transition(state->current_mode, target_mode)) {
            // Valid transition - update state
            state->previous_mode = state->current_mode;
            state->current_mode = target_mode;
            state->mode_entry_time = current_time;
        }
        // If transition is invalid, stay in current mode
    }
    
    return state->current_mode;
}

OperatingMode mode_controller_get_mode(const ModeState* state) {
    if (!state) return MODE_OFF;
    return state->current_mode;
}

OperatingMode mode_controller_get_previous_mode(const ModeState* state) {
    if (!state) return MODE_OFF;
    return state->previous_mode;
}

uint64_t mode_controller_get_time_in_mode(const ModeState* state, uint64_t current_time) {
    if (!state) return 0;
    
    if (current_time >= state->mode_entry_time) {
        return current_time - state->mode_entry_time;
    }
    
    return 0;
}

void mode_controller_set_wakerq(ModeState* state, bool set) {
    if (!state) return;
    state->wakerq_flag = set;
}

bool mode_controller_get_wakerq(const ModeState* state) {
    if (!state) return false;
    return state->wakerq_flag;
}
