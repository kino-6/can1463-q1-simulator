#include <gtest/gtest.h>
#include <rapidcheck.h>
#include "wake_handler.h"

// Test fixture for Wake Handler tests
class WakeHandlerTest : public ::testing::Test {
protected:
    WakeState state;
    
    void SetUp() override {
        wake_handler_init(&state);
    }
};

// Test initialization
TEST_F(WakeHandlerTest, InitializationSetsDefaultValues) {
    EXPECT_FALSE(state.wakerq_flag);
    EXPECT_FALSE(state.wakesr_flag);
    EXPECT_FALSE(state.wake_source_local);
    EXPECT_EQ(state.wup_state, WUP_STATE_IDLE);
    EXPECT_EQ(state.wup_phase_start, UINT64_MAX);
    EXPECT_EQ(state.wup_timeout_start, UINT64_MAX);
    EXPECT_FALSE(state.wake_pin_prev_state);
}

// Test WUP pattern recognition - complete valid pattern
TEST_F(WakeHandlerTest, WUPPatternRecognitionComplete) {
    uint64_t current_time = 0;
    
    // Start with recessive bus (idle)
    wake_handler_update(&state, BUS_STATE_RECESSIVE, false, MODE_SLEEP, current_time);
    EXPECT_EQ(state.wup_state, WUP_STATE_IDLE);
    
    // First dominant phase (>= 0.5μs)
    current_time += 100;  // 0.1μs
    wake_handler_update(&state, BUS_STATE_DOMINANT, false, MODE_SLEEP, current_time);
    EXPECT_EQ(state.wup_state, WUP_STATE_FIRST_DOMINANT);
    
    current_time += 500;  // Total 0.6μs dominant
    wake_handler_update(&state, BUS_STATE_DOMINANT, false, MODE_SLEEP, current_time);
    EXPECT_EQ(state.wup_state, WUP_STATE_RECESSIVE);
    
    // Recessive phase (>= 0.5μs)
    current_time += 100;
    wake_handler_update(&state, BUS_STATE_RECESSIVE, false, MODE_SLEEP, current_time);
    
    current_time += 500;  // Total 0.6μs recessive
    wake_handler_update(&state, BUS_STATE_RECESSIVE, false, MODE_SLEEP, current_time);
    EXPECT_EQ(state.wup_state, WUP_STATE_SECOND_DOMINANT);
    
    // Second dominant phase (>= 0.5μs)
    current_time += 100;
    wake_handler_update(&state, BUS_STATE_DOMINANT, false, MODE_SLEEP, current_time);
    
    current_time += 500;  // Total 0.6μs dominant
    wake_handler_update(&state, BUS_STATE_DOMINANT, false, MODE_SLEEP, current_time);
    
    // WUP pattern complete - WAKERQ should be set
    EXPECT_TRUE(wake_handler_get_wakerq(&state));
    EXPECT_TRUE(wake_handler_get_wakesr(&state));
    EXPECT_FALSE(wake_handler_is_local_wake(&state));  // Remote wake-up
    EXPECT_EQ(state.wup_state, WUP_STATE_COMPLETE);
}

// Test WUP pattern recognition - timeout
TEST_F(WakeHandlerTest, WUPPatternTimeout) {
    uint64_t current_time = 0;
    
    // Start first dominant phase
    wake_handler_update(&state, BUS_STATE_DOMINANT, false, MODE_SLEEP, current_time);
    EXPECT_EQ(state.wup_state, WUP_STATE_FIRST_DOMINANT);
    EXPECT_NE(state.wup_timeout_start, UINT64_MAX);  // Timeout timer should be started
    
    // Advance time beyond timeout (2ms)
    current_time += 3000000;  // 3ms
    wake_handler_update(&state, BUS_STATE_DOMINANT, false, MODE_SLEEP, current_time);
    
    // Should reset to idle due to timeout
    EXPECT_EQ(state.wup_state, WUP_STATE_IDLE);
    EXPECT_FALSE(wake_handler_get_wakerq(&state));
    EXPECT_EQ(state.wup_timeout_start, UINT64_MAX);  // Timeout timer should be cleared
}

// Test WUP pattern recognition - invalid pattern (too short dominant)
TEST_F(WakeHandlerTest, WUPPatternInvalidTooShort) {
    uint64_t current_time = 0;
    
    // Start first dominant phase
    wake_handler_update(&state, BUS_STATE_DOMINANT, false, MODE_SLEEP, current_time);
    EXPECT_EQ(state.wup_state, WUP_STATE_FIRST_DOMINANT);
    
    // Go recessive too early (before filter time)
    current_time += 300;  // 0.3μs (< 0.5μs minimum)
    wake_handler_update(&state, BUS_STATE_RECESSIVE, false, MODE_SLEEP, current_time);
    
    // Should reset to idle
    EXPECT_EQ(state.wup_state, WUP_STATE_IDLE);
    EXPECT_FALSE(wake_handler_get_wakerq(&state));
}

// Test local wake-up (LWU) - rising edge
TEST_F(WakeHandlerTest, LocalWakeUpRisingEdge) {
    uint64_t current_time = 0;
    
    // WAKE pin starts low
    wake_handler_update(&state, BUS_STATE_RECESSIVE, false, MODE_SLEEP, current_time);
    EXPECT_FALSE(wake_handler_get_wakerq(&state));
    
    // WAKE pin goes high (rising edge)
    current_time += 1000;
    wake_handler_update(&state, BUS_STATE_RECESSIVE, true, MODE_SLEEP, current_time);
    
    // WAKERQ should be set, wake source is local
    EXPECT_TRUE(wake_handler_get_wakerq(&state));
    EXPECT_TRUE(wake_handler_get_wakesr(&state));
    EXPECT_TRUE(wake_handler_is_local_wake(&state));
}

// Test local wake-up (LWU) - falling edge
TEST_F(WakeHandlerTest, LocalWakeUpFallingEdge) {
    uint64_t current_time = 0;
    
    // Initialize WAKE pin to high state first (establish previous state)
    state.wake_pin_prev_state = true;
    
    // WAKE pin starts high - update to establish state
    wake_handler_update(&state, BUS_STATE_RECESSIVE, true, MODE_SLEEP, current_time);
    EXPECT_FALSE(wake_handler_get_wakerq(&state));
    
    // WAKE pin goes low (falling edge)
    current_time += 1000;
    wake_handler_update(&state, BUS_STATE_RECESSIVE, false, MODE_SLEEP, current_time);
    
    // WAKERQ should be set, wake source is local
    EXPECT_TRUE(wake_handler_get_wakerq(&state));
    EXPECT_TRUE(wake_handler_get_wakesr(&state));
    EXPECT_TRUE(wake_handler_is_local_wake(&state));
}

// Test that wake-up only works in Sleep mode for LWU
TEST_F(WakeHandlerTest, LocalWakeUpOnlyInSleepMode) {
    uint64_t current_time = 0;
    
    // Try LWU in Normal mode - should not trigger
    wake_handler_update(&state, BUS_STATE_RECESSIVE, false, MODE_NORMAL, current_time);
    current_time += 1000;
    wake_handler_update(&state, BUS_STATE_RECESSIVE, true, MODE_NORMAL, current_time);
    EXPECT_FALSE(wake_handler_get_wakerq(&state));
    
    // Reset state
    wake_handler_init(&state);
    current_time = 0;
    
    // Try LWU in Standby mode - should not trigger (LWU only in Sleep)
    wake_handler_update(&state, BUS_STATE_RECESSIVE, false, MODE_STANDBY, current_time);
    current_time += 1000;
    wake_handler_update(&state, BUS_STATE_RECESSIVE, true, MODE_STANDBY, current_time);
    EXPECT_FALSE(wake_handler_get_wakerq(&state));
}

// Test that WUP works in both Standby and Sleep modes
TEST_F(WakeHandlerTest, WUPWorksInStandbyAndSleepModes) {
    uint64_t current_time = 0;
    
    // Test in Standby mode
    wake_handler_update(&state, BUS_STATE_DOMINANT, false, MODE_STANDBY, current_time);
    current_time += 600;
    wake_handler_update(&state, BUS_STATE_DOMINANT, false, MODE_STANDBY, current_time);
    current_time += 100;
    wake_handler_update(&state, BUS_STATE_RECESSIVE, false, MODE_STANDBY, current_time);
    current_time += 600;
    wake_handler_update(&state, BUS_STATE_RECESSIVE, false, MODE_STANDBY, current_time);
    current_time += 100;
    wake_handler_update(&state, BUS_STATE_DOMINANT, false, MODE_STANDBY, current_time);
    current_time += 600;
    wake_handler_update(&state, BUS_STATE_DOMINANT, false, MODE_STANDBY, current_time);
    
    EXPECT_TRUE(wake_handler_get_wakerq(&state));
    
    // Reset and test in Sleep mode
    wake_handler_init(&state);
    current_time = 0;
    
    wake_handler_update(&state, BUS_STATE_DOMINANT, false, MODE_SLEEP, current_time);
    current_time += 600;
    wake_handler_update(&state, BUS_STATE_DOMINANT, false, MODE_SLEEP, current_time);
    current_time += 100;
    wake_handler_update(&state, BUS_STATE_RECESSIVE, false, MODE_SLEEP, current_time);
    current_time += 600;
    wake_handler_update(&state, BUS_STATE_RECESSIVE, false, MODE_SLEEP, current_time);
    current_time += 100;
    wake_handler_update(&state, BUS_STATE_DOMINANT, false, MODE_SLEEP, current_time);
    current_time += 600;
    wake_handler_update(&state, BUS_STATE_DOMINANT, false, MODE_SLEEP, current_time);
    
    EXPECT_TRUE(wake_handler_get_wakerq(&state));
}

// Test flag clearing
TEST_F(WakeHandlerTest, FlagClearing) {
    uint64_t current_time = 0;
    
    // Trigger local wake-up
    wake_handler_update(&state, BUS_STATE_RECESSIVE, false, MODE_SLEEP, current_time);
    current_time += 1000;
    wake_handler_update(&state, BUS_STATE_RECESSIVE, true, MODE_SLEEP, current_time);
    EXPECT_TRUE(wake_handler_get_wakerq(&state));
    
    // Clear flags
    wake_handler_clear_flags(&state);
    EXPECT_FALSE(wake_handler_get_wakerq(&state));
    // WAKESR persists to indicate wake source
    EXPECT_TRUE(wake_handler_get_wakesr(&state));
}

// Test null pointer handling
TEST_F(WakeHandlerTest, NullPointerHandling) {
    wake_handler_init(nullptr);  // Should not crash
    wake_handler_update(nullptr, BUS_STATE_RECESSIVE, false, MODE_SLEEP, 0);  // Should not crash
    wake_handler_process_wup(nullptr, BUS_STATE_DOMINANT, 0);  // Should not crash
    wake_handler_process_lwu(nullptr, true, 0);  // Should not crash
    wake_handler_clear_flags(nullptr);  // Should not crash
    EXPECT_FALSE(wake_handler_get_wakerq(nullptr));
    EXPECT_FALSE(wake_handler_get_wakesr(nullptr));
    EXPECT_FALSE(wake_handler_is_local_wake(nullptr));
}

// ============================================================================
// Property-Based Tests
// ============================================================================

// Feature: tcan1463-q1-simulator, Property 14: Wake-up pattern recognition
// **Validates: Requirements 6.1, 6.5**
//
// Property: For any bus state sequence in Standby or Sleep mode, when a valid WUP
// pattern (filtered dominant >= tWK_FILTER, filtered recessive >= tWK_FILTER,
// filtered dominant >= tWK_FILTER) is received within tWK_TIMEOUT, the WAKERQ flag
// should be set and the mode should transition to Standby.
TEST(WakeHandlerPropertyTest, WakeUpPatternRecognition) {
    rc::check("Wake-up pattern recognition property", []() {
        WakeState state;
        wake_handler_init(&state);
        
        // Generate random durations for each phase (in nanoseconds)
        // Range: 0 to 2000ns (0 to 2μs, covers below and above tWK_FILTER min of 0.5μs)
        const auto dom1_duration_ns = *rc::gen::inRange(0, 2001);
        const auto rec_duration_ns = *rc::gen::inRange(0, 2001);
        const auto dom2_duration_ns = *rc::gen::inRange(0, 2001);
        
        // Generate random mode (Standby or Sleep)
        const auto mode_val = *rc::gen::inRange(0, 2);
        const OperatingMode mode = (mode_val == 0) ? MODE_STANDBY : MODE_SLEEP;
        
        uint64_t current_time = 0;
        
        // Start with recessive bus
        wake_handler_update(&state, BUS_STATE_RECESSIVE, false, mode, current_time);
        
        // First dominant phase
        wake_handler_update(&state, BUS_STATE_DOMINANT, false, mode, current_time);
        current_time += dom1_duration_ns;
        wake_handler_update(&state, BUS_STATE_DOMINANT, false, mode, current_time);
        
        // Recessive phase
        wake_handler_update(&state, BUS_STATE_RECESSIVE, false, mode, current_time);
        current_time += rec_duration_ns;
        wake_handler_update(&state, BUS_STATE_RECESSIVE, false, mode, current_time);
        
        // Second dominant phase
        wake_handler_update(&state, BUS_STATE_DOMINANT, false, mode, current_time);
        current_time += dom2_duration_ns;
        wake_handler_update(&state, BUS_STATE_DOMINANT, false, mode, current_time);
        
        // Check if pattern is valid (all phases >= tWK_FILTER_MIN_US = 0.5μs = 500ns)
        const uint64_t min_filter_ns = 500;
        bool valid_pattern = (dom1_duration_ns >= min_filter_ns) &&
                            (rec_duration_ns >= min_filter_ns) &&
                            (dom2_duration_ns >= min_filter_ns);
        
        // Check if within timeout (tWK_TIMEOUT_MAX_MS = 2ms = 2000000ns)
        const uint64_t max_timeout_ns = 2000000;
        bool within_timeout = current_time <= max_timeout_ns;
        
        // If valid pattern and within timeout, WAKERQ should be set
        if (valid_pattern && within_timeout) {
            RC_ASSERT(wake_handler_get_wakerq(&state));
            RC_ASSERT(wake_handler_get_wakesr(&state));
            RC_ASSERT(!wake_handler_is_local_wake(&state));  // Remote wake-up
        }
        
        // If pattern is invalid or timeout exceeded, WAKERQ should not be set
        if (!valid_pattern || !within_timeout) {
            RC_ASSERT(!wake_handler_get_wakerq(&state));
        }
    });
}

// Feature: tcan1463-q1-simulator, Property 15: Local wake-up detection
// **Validates: Requirements 6.2**
//
// Property: For any WAKE pin state in Sleep mode, when a transition (rising or
// falling edge) occurs on the WAKE pin, the WAKERQ flag should be set and the
// mode should transition to Standby.
TEST(WakeHandlerPropertyTest, LocalWakeUpDetection) {
    rc::check("Local wake-up detection property", []() {
        WakeState state;
        wake_handler_init(&state);
        
        // Generate random initial WAKE pin state
        const auto initial_wake_state = *rc::gen::inRange(0, 2) == 1;
        
        // Generate random final WAKE pin state (must be different for edge)
        const auto final_wake_state = !initial_wake_state;
        
        // Generate random time delay
        const auto time_delay_ns = *rc::gen::inRange(1, 100001);
        
        uint64_t current_time = 0;
        
        // Initialize the previous state
        state.wake_pin_prev_state = initial_wake_state;
        
        // Set initial WAKE pin state (this establishes the baseline)
        wake_handler_update(&state, BUS_STATE_RECESSIVE, initial_wake_state, 
                          MODE_SLEEP, current_time);
        
        // Verify WAKERQ is not set yet
        RC_ASSERT(!wake_handler_get_wakerq(&state));
        
        // Transition WAKE pin (edge detection)
        current_time += time_delay_ns;
        wake_handler_update(&state, BUS_STATE_RECESSIVE, final_wake_state,
                          MODE_SLEEP, current_time);
        
        // WAKERQ should be set after edge detection
        RC_ASSERT(wake_handler_get_wakerq(&state));
        RC_ASSERT(wake_handler_get_wakesr(&state));
        RC_ASSERT(wake_handler_is_local_wake(&state));  // Local wake-up
    });
}

// Feature: tcan1463-q1-simulator, Property 16: Wake-up flag indication
// **Validates: Requirements 6.3**
//
// Property: For any state, when WAKERQ flag is set, RXD and nFAULT pins should
// both be driven low.
// Note: This property tests the flag state; pin control is tested in integration tests
TEST(WakeHandlerPropertyTest, WakeUpFlagIndication) {
    rc::check("Wake-up flag indication property", []() {
        WakeState state;
        wake_handler_init(&state);
        
        // Generate random wake-up trigger (0 = remote, 1 = local)
        const auto wake_type = *rc::gen::inRange(0, 2);
        
        uint64_t current_time = 0;
        
        if (wake_type == 0) {
            // Remote wake-up (WUP pattern)
            // Generate valid WUP pattern
            wake_handler_update(&state, BUS_STATE_DOMINANT, false, MODE_SLEEP, current_time);
            current_time += 600;  // 0.6μs
            wake_handler_update(&state, BUS_STATE_DOMINANT, false, MODE_SLEEP, current_time);
            wake_handler_update(&state, BUS_STATE_RECESSIVE, false, MODE_SLEEP, current_time);
            current_time += 600;
            wake_handler_update(&state, BUS_STATE_RECESSIVE, false, MODE_SLEEP, current_time);
            wake_handler_update(&state, BUS_STATE_DOMINANT, false, MODE_SLEEP, current_time);
            current_time += 600;
            wake_handler_update(&state, BUS_STATE_DOMINANT, false, MODE_SLEEP, current_time);
        } else {
            // Local wake-up (WAKE pin edge)
            wake_handler_update(&state, BUS_STATE_RECESSIVE, false, MODE_SLEEP, current_time);
            current_time += 1000;
            wake_handler_update(&state, BUS_STATE_RECESSIVE, true, MODE_SLEEP, current_time);
        }
        
        // WAKERQ should be set
        RC_ASSERT(wake_handler_get_wakerq(&state));
        
        // Note: The actual pin control (RXD and nFAULT driven low) is handled
        // by the main simulator based on the WAKERQ flag state.
        // This property verifies that the flag is correctly set.
    });
}

// Feature: tcan1463-q1-simulator, Property 17: Wake-up flag clearing
// **Validates: Requirements 6.4**
//
// Property: For any state with WAKERQ flag set, when transitioning to Normal mode,
// the WAKERQ flag should be cleared.
TEST(WakeHandlerPropertyTest, WakeUpFlagClearing) {
    rc::check("Wake-up flag clearing property", []() {
        WakeState state;
        wake_handler_init(&state);
        
        // Generate random wake-up trigger to set WAKERQ
        const auto wake_type = *rc::gen::inRange(0, 2);
        
        uint64_t current_time = 0;
        
        if (wake_type == 0) {
            // Remote wake-up
            wake_handler_update(&state, BUS_STATE_DOMINANT, false, MODE_SLEEP, current_time);
            current_time += 600;
            wake_handler_update(&state, BUS_STATE_DOMINANT, false, MODE_SLEEP, current_time);
            wake_handler_update(&state, BUS_STATE_RECESSIVE, false, MODE_SLEEP, current_time);
            current_time += 600;
            wake_handler_update(&state, BUS_STATE_RECESSIVE, false, MODE_SLEEP, current_time);
            wake_handler_update(&state, BUS_STATE_DOMINANT, false, MODE_SLEEP, current_time);
            current_time += 600;
            wake_handler_update(&state, BUS_STATE_DOMINANT, false, MODE_SLEEP, current_time);
        } else {
            // Local wake-up
            wake_handler_update(&state, BUS_STATE_RECESSIVE, false, MODE_SLEEP, current_time);
            current_time += 1000;
            wake_handler_update(&state, BUS_STATE_RECESSIVE, true, MODE_SLEEP, current_time);
        }
        
        // Verify WAKERQ is set
        RC_ASSERT(wake_handler_get_wakerq(&state));
        
        // Clear flags (simulating transition to Normal mode)
        wake_handler_clear_flags(&state);
        
        // WAKERQ should be cleared
        RC_ASSERT(!wake_handler_get_wakerq(&state));
        
        // WAKESR should persist (not cleared)
        RC_ASSERT(wake_handler_get_wakesr(&state));
    });
}
