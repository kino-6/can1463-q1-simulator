#include <gtest/gtest.h>
#include <rapidcheck.h>
#include "mode_controller.h"

// Test fixture for Mode Controller tests
class ModeControllerTest : public ::testing::Test {
protected:
    ModeState state;
    
    void SetUp() override {
        mode_controller_init(&state);
    }
};

// Test initialization
TEST_F(ModeControllerTest, Initialization) {
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_OFF);
    EXPECT_EQ(mode_controller_get_previous_mode(&state), MODE_OFF);
    EXPECT_FALSE(mode_controller_get_wakerq(&state));
}

// Test WAKERQ flag management
TEST_F(ModeControllerTest, WakerqFlagManagement) {
    EXPECT_FALSE(mode_controller_get_wakerq(&state));
    
    mode_controller_set_wakerq(&state, true);
    EXPECT_TRUE(mode_controller_get_wakerq(&state));
    
    mode_controller_set_wakerq(&state, false);
    EXPECT_FALSE(mode_controller_get_wakerq(&state));
}

// Test Off to Normal transition
TEST_F(ModeControllerTest, OffToNormalTransition) {
    // Start in Off mode
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_OFF);
    
    // Apply conditions for Normal mode: VSUP valid, EN=H, nSTB=H
    OperatingMode mode = mode_controller_update(&state, true, true, true, false, 0);
    
    EXPECT_EQ(mode, MODE_NORMAL);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_NORMAL);
    EXPECT_EQ(mode_controller_get_previous_mode(&state), MODE_OFF);
}

// Test Off to Silent transition
TEST_F(ModeControllerTest, OffToSilentTransition) {
    // Start in Off mode
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_OFF);
    
    // Apply conditions for Silent mode: VSUP valid, EN=L, nSTB=H
    OperatingMode mode = mode_controller_update(&state, false, true, true, false, 0);
    
    EXPECT_EQ(mode, MODE_SILENT);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_SILENT);
}

// Test Normal to Silent transition
TEST_F(ModeControllerTest, NormalToSilentTransition) {
    // First go to Normal mode
    mode_controller_update(&state, true, true, true, false, 0);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_NORMAL);
    
    // Change EN to low
    OperatingMode mode = mode_controller_update(&state, false, true, true, false, 1000);
    
    EXPECT_EQ(mode, MODE_SILENT);
    EXPECT_EQ(mode_controller_get_previous_mode(&state), MODE_NORMAL);
}

// Test Silent to Normal transition
TEST_F(ModeControllerTest, SilentToNormalTransition) {
    // First go to Silent mode
    mode_controller_update(&state, false, true, true, false, 0);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_SILENT);
    
    // Change EN to high
    OperatingMode mode = mode_controller_update(&state, true, true, true, false, 1000);
    
    EXPECT_EQ(mode, MODE_NORMAL);
    EXPECT_EQ(mode_controller_get_previous_mode(&state), MODE_SILENT);
}

// Test Normal to Standby transition
TEST_F(ModeControllerTest, NormalToStandbyTransition) {
    // First go to Normal mode
    mode_controller_update(&state, true, true, true, false, 0);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_NORMAL);
    
    // Set WAKERQ and pull nSTB low
    mode_controller_set_wakerq(&state, true);
    OperatingMode mode = mode_controller_update(&state, true, false, true, true, 1000);
    
    EXPECT_EQ(mode, MODE_STANDBY);
    EXPECT_EQ(mode_controller_get_previous_mode(&state), MODE_NORMAL);
}

// Test Normal to Go-to-sleep transition
TEST_F(ModeControllerTest, NormalToGoToSleepTransition) {
    // First go to Normal mode
    mode_controller_update(&state, true, true, true, false, 0);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_NORMAL);
    
    // Pull nSTB low without WAKERQ
    OperatingMode mode = mode_controller_update(&state, true, false, true, false, 1000);
    
    EXPECT_EQ(mode, MODE_GO_TO_SLEEP);
    EXPECT_EQ(mode_controller_get_previous_mode(&state), MODE_NORMAL);
}

// Test Go-to-sleep to Sleep transition (after tSILENCE)
TEST_F(ModeControllerTest, GoToSleepToSleepTransition) {
    // First go to Go-to-sleep mode
    mode_controller_update(&state, true, true, true, false, 0);
    mode_controller_update(&state, true, false, true, false, 1000);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_GO_TO_SLEEP);
    
    // Wait for tSILENCE (0.6s minimum = 600,000,000 ns)
    uint64_t entry_time = state.mode_entry_time;
    uint64_t after_silence = entry_time + 600000000ULL + 1000;
    
    OperatingMode mode = mode_controller_update(&state, true, false, true, false, after_silence);
    
    EXPECT_EQ(mode, MODE_SLEEP);
    EXPECT_EQ(mode_controller_get_previous_mode(&state), MODE_GO_TO_SLEEP);
}

// Test Standby to Normal transition
TEST_F(ModeControllerTest, StandbyToNormalTransition) {
    // First go to Standby mode
    mode_controller_update(&state, true, true, true, false, 0);
    mode_controller_set_wakerq(&state, true);
    mode_controller_update(&state, true, false, true, true, 1000);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_STANDBY);
    
    // Pull nSTB high with EN high
    OperatingMode mode = mode_controller_update(&state, true, true, true, true, 2000);
    
    EXPECT_EQ(mode, MODE_NORMAL);
    EXPECT_EQ(mode_controller_get_previous_mode(&state), MODE_STANDBY);
}

// Test Standby to Silent transition
TEST_F(ModeControllerTest, StandbyToSilentTransition) {
    // First go to Standby mode
    mode_controller_update(&state, false, true, true, false, 0);
    mode_controller_set_wakerq(&state, true);
    mode_controller_update(&state, false, false, true, true, 1000);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_STANDBY);
    
    // Pull nSTB high with EN low
    OperatingMode mode = mode_controller_update(&state, false, true, true, true, 2000);
    
    EXPECT_EQ(mode, MODE_SILENT);
    EXPECT_EQ(mode_controller_get_previous_mode(&state), MODE_STANDBY);
}

// Test Sleep to Standby transition (wake-up)
TEST_F(ModeControllerTest, SleepToStandbyTransition) {
    // First go to Sleep mode
    mode_controller_update(&state, true, true, true, false, 0);
    mode_controller_update(&state, true, false, true, false, 1000);
    uint64_t entry_time = state.mode_entry_time;
    mode_controller_update(&state, true, false, true, false, entry_time + 700000000ULL);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_SLEEP);
    
    // Set WAKERQ (wake-up event)
    mode_controller_set_wakerq(&state, true);
    OperatingMode mode = mode_controller_update(&state, true, false, true, true, entry_time + 800000000ULL);
    
    EXPECT_EQ(mode, MODE_STANDBY);
    EXPECT_EQ(mode_controller_get_previous_mode(&state), MODE_SLEEP);
}

// Test any mode to Off transition (power loss)
TEST_F(ModeControllerTest, AnyModeToOffTransition) {
    // Test from Normal
    mode_controller_update(&state, true, true, true, false, 0);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_NORMAL);
    
    OperatingMode mode = mode_controller_update(&state, true, true, false, false, 1000);
    EXPECT_EQ(mode, MODE_OFF);
    
    // Test from Silent
    mode_controller_update(&state, false, true, true, false, 2000);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_SILENT);
    
    mode = mode_controller_update(&state, false, true, false, false, 3000);
    EXPECT_EQ(mode, MODE_OFF);
}

// Test mode transition validation
TEST_F(ModeControllerTest, TransitionValidation) {
    // Valid transitions
    EXPECT_TRUE(mode_controller_can_transition(MODE_OFF, MODE_NORMAL));
    EXPECT_TRUE(mode_controller_can_transition(MODE_OFF, MODE_SILENT));
    EXPECT_TRUE(mode_controller_can_transition(MODE_NORMAL, MODE_SILENT));
    EXPECT_TRUE(mode_controller_can_transition(MODE_SILENT, MODE_NORMAL));
    EXPECT_TRUE(mode_controller_can_transition(MODE_NORMAL, MODE_STANDBY));
    EXPECT_TRUE(mode_controller_can_transition(MODE_NORMAL, MODE_GO_TO_SLEEP));
    EXPECT_TRUE(mode_controller_can_transition(MODE_GO_TO_SLEEP, MODE_SLEEP));
    EXPECT_TRUE(mode_controller_can_transition(MODE_STANDBY, MODE_NORMAL));
    EXPECT_TRUE(mode_controller_can_transition(MODE_SLEEP, MODE_STANDBY));
    EXPECT_TRUE(mode_controller_can_transition(MODE_NORMAL, MODE_OFF));
    
    // Same mode is always valid
    EXPECT_TRUE(mode_controller_can_transition(MODE_NORMAL, MODE_NORMAL));
    
    // Invalid transitions
    EXPECT_FALSE(mode_controller_can_transition(MODE_OFF, MODE_STANDBY));
    EXPECT_FALSE(mode_controller_can_transition(MODE_OFF, MODE_SLEEP));
    EXPECT_FALSE(mode_controller_can_transition(MODE_NORMAL, MODE_SLEEP));
    EXPECT_FALSE(mode_controller_can_transition(MODE_SLEEP, MODE_NORMAL));
    EXPECT_FALSE(mode_controller_can_transition(MODE_SLEEP, MODE_GO_TO_SLEEP));
}

// Test time in mode calculation
TEST_F(ModeControllerTest, TimeInModeCalculation) {
    mode_controller_update(&state, true, true, true, false, 1000);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_NORMAL);
    
    uint64_t time_in_mode = mode_controller_get_time_in_mode(&state, 1000);
    EXPECT_EQ(time_in_mode, 0ULL);
    
    time_in_mode = mode_controller_get_time_in_mode(&state, 1000000);
    EXPECT_EQ(time_in_mode, 999000ULL);
    
    time_in_mode = mode_controller_get_time_in_mode(&state, 2000000);
    EXPECT_EQ(time_in_mode, 1999000ULL);
}

// Test that Go-to-sleep doesn't transition to Sleep before tSILENCE
TEST_F(ModeControllerTest, GoToSleepDoesNotTransitionEarly) {
    // Go to Go-to-sleep mode
    mode_controller_update(&state, true, true, true, false, 0);
    mode_controller_update(&state, true, false, true, false, 1000);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_GO_TO_SLEEP);
    
    uint64_t entry_time = state.mode_entry_time;
    
    // Check at various times before tSILENCE
    OperatingMode mode = mode_controller_update(&state, true, false, true, false, entry_time + 100000000ULL);
    EXPECT_EQ(mode, MODE_GO_TO_SLEEP);
    
    mode = mode_controller_update(&state, true, false, true, false, entry_time + 500000000ULL);
    EXPECT_EQ(mode, MODE_GO_TO_SLEEP);
    
    // Just before tSILENCE (599ms)
    mode = mode_controller_update(&state, true, false, true, false, entry_time + 599000000ULL);
    EXPECT_EQ(mode, MODE_GO_TO_SLEEP);
}

// Test that Sleep mode stays in Sleep when conditions don't change
TEST_F(ModeControllerTest, SleepModeStability) {
    // Go to Sleep mode
    mode_controller_update(&state, true, true, true, false, 0);
    mode_controller_update(&state, true, false, true, false, 1000);
    uint64_t entry_time = state.mode_entry_time;
    mode_controller_update(&state, true, false, true, false, entry_time + 700000000ULL);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_SLEEP);
    
    // Stay in Sleep mode
    OperatingMode mode = mode_controller_update(&state, true, false, true, false, entry_time + 800000000ULL);
    EXPECT_EQ(mode, MODE_SLEEP);
    
    mode = mode_controller_update(&state, true, false, true, false, entry_time + 1000000000ULL);
    EXPECT_EQ(mode, MODE_SLEEP);
}

// ============================================================================
// Additional Mode Transition Tests (Requirements 2.1-2.6)
// ============================================================================

// Test Silent to Standby transition
TEST_F(ModeControllerTest, SilentToStandbyTransition) {
    // First go to Silent mode
    mode_controller_update(&state, false, true, true, false, 0);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_SILENT);
    
    // Set WAKERQ and pull nSTB low
    mode_controller_set_wakerq(&state, true);
    OperatingMode mode = mode_controller_update(&state, false, false, true, true, 1000);
    
    EXPECT_EQ(mode, MODE_STANDBY);
    EXPECT_EQ(mode_controller_get_previous_mode(&state), MODE_SILENT);
}

// Test Silent to Go-to-sleep transition
TEST_F(ModeControllerTest, SilentToGoToSleepTransition) {
    // First go to Silent mode
    mode_controller_update(&state, false, true, true, false, 0);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_SILENT);
    
    // Pull nSTB low without WAKERQ
    OperatingMode mode = mode_controller_update(&state, false, false, true, false, 1000);
    
    EXPECT_EQ(mode, MODE_GO_TO_SLEEP);
    EXPECT_EQ(mode_controller_get_previous_mode(&state), MODE_SILENT);
}

// Test that invalid transition from Off to Standby is rejected
TEST_F(ModeControllerTest, InvalidOffToStandbyRejected) {
    // Start in Off mode
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_OFF);
    
    // Try to go directly to Standby (should not be possible)
    EXPECT_FALSE(mode_controller_can_transition(MODE_OFF, MODE_STANDBY));
    
    // Manually set state to verify rejection
    state.current_mode = MODE_OFF;
    mode_controller_set_wakerq(&state, true);
    
    // Even with WAKERQ set and nSTB low, should not go to Standby from Off
    // because VSUP is not valid
    OperatingMode mode = mode_controller_update(&state, true, false, false, true, 1000);
    EXPECT_EQ(mode, MODE_OFF);
}

// Test that invalid transition from Off to Go-to-sleep is rejected
TEST_F(ModeControllerTest, InvalidOffToGoToSleepRejected) {
    // Start in Off mode
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_OFF);
    
    // Verify transition is invalid
    EXPECT_FALSE(mode_controller_can_transition(MODE_OFF, MODE_GO_TO_SLEEP));
}

// Test that invalid transition from Off to Sleep is rejected
TEST_F(ModeControllerTest, InvalidOffToSleepRejected) {
    // Start in Off mode
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_OFF);
    
    // Verify transition is invalid
    EXPECT_FALSE(mode_controller_can_transition(MODE_OFF, MODE_SLEEP));
}

// Test that invalid transition from Normal to Sleep is rejected
TEST_F(ModeControllerTest, InvalidNormalToSleepRejected) {
    // Go to Normal mode
    mode_controller_update(&state, true, true, true, false, 0);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_NORMAL);
    
    // Verify transition is invalid
    EXPECT_FALSE(mode_controller_can_transition(MODE_NORMAL, MODE_SLEEP));
    
    // Cannot go directly to Sleep from Normal
    // Must go through Go-to-sleep first
}

// Test that invalid transition from Silent to Sleep is rejected
TEST_F(ModeControllerTest, InvalidSilentToSleepRejected) {
    // Go to Silent mode
    mode_controller_update(&state, false, true, true, false, 0);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_SILENT);
    
    // Verify transition is invalid
    EXPECT_FALSE(mode_controller_can_transition(MODE_SILENT, MODE_SLEEP));
}

// Test that invalid transition from Sleep to Normal is rejected
TEST_F(ModeControllerTest, InvalidSleepToNormalRejected) {
    // Go to Sleep mode
    mode_controller_update(&state, true, true, true, false, 0);
    mode_controller_update(&state, true, false, true, false, 1000);
    uint64_t entry_time = state.mode_entry_time;
    mode_controller_update(&state, true, false, true, false, entry_time + 700000000ULL);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_SLEEP);
    
    // Verify transition is invalid
    EXPECT_FALSE(mode_controller_can_transition(MODE_SLEEP, MODE_NORMAL));
    
    // Cannot go directly to Normal from Sleep
    // Must go through Standby first
}

// Test that invalid transition from Sleep to Silent is rejected
TEST_F(ModeControllerTest, InvalidSleepToSilentRejected) {
    // Go to Sleep mode
    mode_controller_update(&state, true, true, true, false, 0);
    mode_controller_update(&state, true, false, true, false, 1000);
    uint64_t entry_time = state.mode_entry_time;
    mode_controller_update(&state, true, false, true, false, entry_time + 700000000ULL);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_SLEEP);
    
    // Verify transition is invalid
    EXPECT_FALSE(mode_controller_can_transition(MODE_SLEEP, MODE_SILENT));
}

// Test that invalid transition from Sleep to Go-to-sleep is rejected
TEST_F(ModeControllerTest, InvalidSleepToGoToSleepRejected) {
    // Go to Sleep mode
    mode_controller_update(&state, true, true, true, false, 0);
    mode_controller_update(&state, true, false, true, false, 1000);
    uint64_t entry_time = state.mode_entry_time;
    mode_controller_update(&state, true, false, true, false, entry_time + 700000000ULL);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_SLEEP);
    
    // Verify transition is invalid
    EXPECT_FALSE(mode_controller_can_transition(MODE_SLEEP, MODE_GO_TO_SLEEP));
}

// Test that invalid transition from Go-to-sleep to Normal is rejected
TEST_F(ModeControllerTest, InvalidGoToSleepToNormalRejected) {
    // Go to Go-to-sleep mode
    mode_controller_update(&state, true, true, true, false, 0);
    mode_controller_update(&state, true, false, true, false, 1000);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_GO_TO_SLEEP);
    
    // Verify transition is invalid
    EXPECT_FALSE(mode_controller_can_transition(MODE_GO_TO_SLEEP, MODE_NORMAL));
}

// Test that invalid transition from Go-to-sleep to Silent is rejected
TEST_F(ModeControllerTest, InvalidGoToSleepToSilentRejected) {
    // Go to Go-to-sleep mode
    mode_controller_update(&state, true, true, true, false, 0);
    mode_controller_update(&state, true, false, true, false, 1000);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_GO_TO_SLEEP);
    
    // Verify transition is invalid
    EXPECT_FALSE(mode_controller_can_transition(MODE_GO_TO_SLEEP, MODE_SILENT));
}

// Test that invalid transition from Go-to-sleep to Standby is rejected
TEST_F(ModeControllerTest, InvalidGoToSleepToStandbyRejected) {
    // Go to Go-to-sleep mode
    mode_controller_update(&state, true, true, true, false, 0);
    mode_controller_update(&state, true, false, true, false, 1000);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_GO_TO_SLEEP);
    
    // Verify transition is invalid
    EXPECT_FALSE(mode_controller_can_transition(MODE_GO_TO_SLEEP, MODE_STANDBY));
}

// Test that invalid transition from Standby to Go-to-sleep is rejected
TEST_F(ModeControllerTest, InvalidStandbyToGoToSleepRejected) {
    // Go to Standby mode
    mode_controller_update(&state, true, true, true, false, 0);
    mode_controller_set_wakerq(&state, true);
    mode_controller_update(&state, true, false, true, true, 1000);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_STANDBY);
    
    // Verify transition is invalid
    EXPECT_FALSE(mode_controller_can_transition(MODE_STANDBY, MODE_GO_TO_SLEEP));
}

// Test that invalid transition from Standby to Sleep is rejected
TEST_F(ModeControllerTest, InvalidStandbyToSleepRejected) {
    // Go to Standby mode
    mode_controller_update(&state, true, true, true, false, 0);
    mode_controller_set_wakerq(&state, true);
    mode_controller_update(&state, true, false, true, true, 1000);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_STANDBY);
    
    // Verify transition is invalid
    EXPECT_FALSE(mode_controller_can_transition(MODE_STANDBY, MODE_SLEEP));
}

// Test mode entry time is updated on transition
TEST_F(ModeControllerTest, ModeEntryTimeUpdated) {
    // Start in Off mode at time 0
    EXPECT_EQ(state.mode_entry_time, 0ULL);
    
    // Transition to Normal at time 1000
    mode_controller_update(&state, true, true, true, false, 1000);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_NORMAL);
    EXPECT_EQ(state.mode_entry_time, 1000ULL);
    
    // Transition to Silent at time 5000
    mode_controller_update(&state, false, true, true, false, 5000);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_SILENT);
    EXPECT_EQ(state.mode_entry_time, 5000ULL);
    
    // Transition to Standby at time 10000
    mode_controller_set_wakerq(&state, true);
    mode_controller_update(&state, false, false, true, true, 10000);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_STANDBY);
    EXPECT_EQ(state.mode_entry_time, 10000ULL);
}

// Test previous mode is tracked correctly
TEST_F(ModeControllerTest, PreviousModeTracking) {
    // Off → Normal
    mode_controller_update(&state, true, true, true, false, 1000);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_NORMAL);
    EXPECT_EQ(mode_controller_get_previous_mode(&state), MODE_OFF);
    
    // Normal → Silent
    mode_controller_update(&state, false, true, true, false, 2000);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_SILENT);
    EXPECT_EQ(mode_controller_get_previous_mode(&state), MODE_NORMAL);
    
    // Silent → Go-to-sleep
    mode_controller_update(&state, false, false, true, false, 3000);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_GO_TO_SLEEP);
    EXPECT_EQ(mode_controller_get_previous_mode(&state), MODE_SILENT);
    
    // Go-to-sleep → Sleep
    uint64_t entry_time = state.mode_entry_time;
    mode_controller_update(&state, false, false, true, false, entry_time + 700000000ULL);
    EXPECT_EQ(mode_controller_get_mode(&state), MODE_SLEEP);
    EXPECT_EQ(mode_controller_get_previous_mode(&state), MODE_GO_TO_SLEEP);
}

// ============================================================================
// Property-Based Tests
// ============================================================================

// Feature: tcan1463-q1-simulator, Property 3: Mode transition validity
// **Validates: Requirements 2.7**
//
// Property: For any current mode and target mode, when a mode transition is
// requested, the transition should only succeed if there exists a valid
// transition path in the mode transition table.
TEST(ModeControllerPropertyTest, ModeTransitionValidity) {
    rc::check("Mode transition validity property", []() {
        ModeState state;
        mode_controller_init(&state);
        
        // Generate random current mode (0-5)
        const auto current_mode_value = *rc::gen::inRange(0, 6);
        const OperatingMode current_mode = (OperatingMode)current_mode_value;
        
        // Generate random target mode (0-5)
        const auto target_mode_value = *rc::gen::inRange(0, 6);
        const OperatingMode target_mode = (OperatingMode)target_mode_value;
        
        // Set the state to the current mode
        state.current_mode = current_mode;
        state.previous_mode = MODE_OFF;
        state.mode_entry_time = 1000;
        
        // Check if the transition is valid according to the transition table
        const bool is_valid_transition = mode_controller_can_transition(current_mode, target_mode);
        
        // Now attempt to trigger the transition by calling mode_controller_update
        // with appropriate inputs for the target mode
        bool en_high = false;
        bool nstb_high = false;
        bool vsup_valid = true;
        bool wakerq_set = false;
        uint64_t current_time = 2000;
        
        // Set inputs based on target mode to attempt the transition
        switch (target_mode) {
            case MODE_OFF:
                vsup_valid = false;  // Power loss
                break;
            case MODE_NORMAL:
                en_high = true;
                nstb_high = true;
                vsup_valid = true;
                break;
            case MODE_SILENT:
                en_high = false;
                nstb_high = true;
                vsup_valid = true;
                break;
            case MODE_STANDBY:
                nstb_high = false;
                wakerq_set = true;
                vsup_valid = true;
                break;
            case MODE_GO_TO_SLEEP:
                nstb_high = false;
                wakerq_set = false;
                vsup_valid = true;
                break;
            case MODE_SLEEP:
                // Sleep requires being in Go-to-sleep for tSILENCE duration
                // Set up the state to be in Go-to-sleep mode
                if (current_mode == MODE_GO_TO_SLEEP) {
                    // Set time to be after tSILENCE (0.6s = 600,000,000 ns)
                    current_time = state.mode_entry_time + 600000001ULL;
                }
                nstb_high = false;
                wakerq_set = false;
                vsup_valid = true;
                break;
        }
        
        // Perform the update
        const OperatingMode result_mode = mode_controller_update(
            &state, en_high, nstb_high, vsup_valid, wakerq_set, current_time
        );
        
        // Property: The transition should only succeed if it's valid
        if (is_valid_transition) {
            // If the transition is valid, the mode should change to target_mode
            // (or stay the same if current_mode == target_mode)
            if (current_mode == target_mode) {
                RC_ASSERT(result_mode == current_mode);
            } else {
                // For valid transitions, the mode should change
                // However, some transitions require specific conditions
                // that may not be met by our simple input setup
                
                // Special case: Sleep can only be reached from Go-to-sleep after timeout
                if (target_mode == MODE_SLEEP && current_mode != MODE_GO_TO_SLEEP) {
                    // This transition requires going through Go-to-sleep first
                    // So we don't expect it to succeed directly
                    RC_SUCCEED();
                    return;
                }
                
                // For other valid transitions, check if the mode changed
                // or if there's a valid reason it didn't
                if (result_mode != target_mode) {
                    // The mode didn't change to target - this could be because:
                    // 1. The inputs don't match the target mode requirements
                    // 2. There's an intermediate state required
                    
                    // Verify that the result mode is either:
                    // - The current mode (no transition)
                    // - A valid intermediate mode
                    RC_ASSERT(result_mode == current_mode || 
                             mode_controller_can_transition(current_mode, result_mode));
                }
            }
        } else {
            // If the transition is invalid, the mode should NOT change to target_mode
            // (unless target_mode == current_mode, which is always valid)
            if (current_mode != target_mode) {
                RC_ASSERT(result_mode != target_mode);
            }
        }
    });
}

// Additional property test: Verify transition table consistency
// This test verifies that the mode_controller_can_transition function
// correctly implements the mode transition table from the design document
TEST(ModeControllerPropertyTest, TransitionTableConsistency) {
    rc::check("Transition table consistency", []() {
        // Generate two random modes
        const auto from_mode_value = *rc::gen::inRange(0, 6);
        const auto to_mode_value = *rc::gen::inRange(0, 6);
        const OperatingMode from_mode = (OperatingMode)from_mode_value;
        const OperatingMode to_mode = (OperatingMode)to_mode_value;
        
        // Check if transition is valid
        const bool is_valid = mode_controller_can_transition(from_mode, to_mode);
        
        // Property 1: Same mode is always valid (no transition)
        if (from_mode == to_mode) {
            RC_ASSERT(is_valid);
        }
        
        // Property 2: Verify specific invalid transitions
        // These are transitions that should NEVER be valid according to the design
        
        // Cannot go directly from Off to Standby, Go-to-sleep, or Sleep
        if (from_mode == MODE_OFF) {
            if (to_mode == MODE_STANDBY || to_mode == MODE_GO_TO_SLEEP || to_mode == MODE_SLEEP) {
                RC_ASSERT(!is_valid);
            }
        }
        
        // Cannot go directly from Normal/Silent to Sleep
        if ((from_mode == MODE_NORMAL || from_mode == MODE_SILENT) && to_mode == MODE_SLEEP) {
            RC_ASSERT(!is_valid);
        }
        
        // Cannot go from Sleep directly to Normal or Silent (must go through Standby)
        if (from_mode == MODE_SLEEP) {
            if (to_mode == MODE_NORMAL || to_mode == MODE_SILENT || to_mode == MODE_GO_TO_SLEEP) {
                RC_ASSERT(!is_valid);
            }
        }
        
        // Cannot go from Go-to-sleep to Normal, Silent, or Standby
        // (must either go to Sleep or back to Off)
        if (from_mode == MODE_GO_TO_SLEEP) {
            if (to_mode == MODE_NORMAL || to_mode == MODE_SILENT || to_mode == MODE_STANDBY) {
                RC_ASSERT(!is_valid);
            }
        }
        
        // Cannot go from Standby to Go-to-sleep or Sleep
        if (from_mode == MODE_STANDBY) {
            if (to_mode == MODE_GO_TO_SLEEP || to_mode == MODE_SLEEP) {
                RC_ASSERT(!is_valid);
            }
        }
    });
}
