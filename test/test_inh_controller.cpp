#include <gtest/gtest.h>
#include <rapidcheck.h>
#include "inh_controller.h"

// Test fixture for INH controller tests
class INHControllerTest : public ::testing::Test {
protected:
    INHController controller;
    
    void SetUp() override {
        inh_controller_init(&controller);
    }
};

// Unit test: INH initialization
TEST_F(INHControllerTest, Initialization) {
    EXPECT_TRUE(controller.inh_enabled);
    EXPECT_FALSE(controller.inh_output_high);
    EXPECT_EQ(controller.wake_event_time, 0ULL);
    EXPECT_FALSE(controller.pending_inh_assertion);
}

// Unit test: INH disabled by INH_MASK high
TEST_F(INHControllerTest, INHMaskDisablesINH) {
    uint64_t current_time = 0;
    
    // Set INH_MASK high
    inh_controller_update(&controller, MODE_NORMAL, true, false, current_time);
    
    EXPECT_FALSE(controller.inh_enabled);
    EXPECT_FALSE(controller.inh_output_high);
    
    PinState state;
    double voltage;
    inh_controller_get_pin_state(&controller, &state, &voltage);
    
    EXPECT_EQ(state, PIN_STATE_HIGH_IMPEDANCE);
}

// Unit test: INH enabled in Normal mode
TEST_F(INHControllerTest, INHHighInNormalMode) {
    uint64_t current_time = 0;
    
    // INH_MASK low, Normal mode
    inh_controller_update(&controller, MODE_NORMAL, false, false, current_time);
    
    EXPECT_TRUE(controller.inh_enabled);
    EXPECT_TRUE(controller.inh_output_high);
    
    PinState state;
    double voltage;
    inh_controller_get_pin_state(&controller, &state, &voltage);
    
    EXPECT_EQ(state, PIN_STATE_HIGH);
    EXPECT_GT(voltage, 0.0);
}

// Unit test: INH enabled in Silent mode
TEST_F(INHControllerTest, INHHighInSilentMode) {
    uint64_t current_time = 0;
    
    inh_controller_update(&controller, MODE_SILENT, false, false, current_time);
    
    EXPECT_TRUE(controller.inh_enabled);
    EXPECT_TRUE(controller.inh_output_high);
}

// Unit test: INH enabled in Standby mode
TEST_F(INHControllerTest, INHHighInStandbyMode) {
    uint64_t current_time = 0;
    
    inh_controller_update(&controller, MODE_STANDBY, false, false, current_time);
    
    EXPECT_TRUE(controller.inh_enabled);
    EXPECT_TRUE(controller.inh_output_high);
}

// Unit test: INH high-Z in Sleep mode
TEST_F(INHControllerTest, INHHighZInSleepMode) {
    uint64_t current_time = 0;
    
    inh_controller_update(&controller, MODE_SLEEP, false, false, current_time);
    
    EXPECT_TRUE(controller.inh_enabled);
    EXPECT_FALSE(controller.inh_output_high);
    
    PinState state;
    double voltage;
    inh_controller_get_pin_state(&controller, &state, &voltage);
    
    EXPECT_EQ(state, PIN_STATE_HIGH_IMPEDANCE);
}

// Unit test: INH high-Z in Go-to-sleep mode
TEST_F(INHControllerTest, INHHighZInGoToSleepMode) {
    uint64_t current_time = 0;
    
    inh_controller_update(&controller, MODE_GO_TO_SLEEP, false, false, current_time);
    
    EXPECT_FALSE(controller.inh_output_high);
}

// Unit test: INH high-Z in Off mode
TEST_F(INHControllerTest, INHHighZInOffMode) {
    uint64_t current_time = 0;
    
    inh_controller_update(&controller, MODE_OFF, false, false, current_time);
    
    EXPECT_FALSE(controller.inh_output_high);
}

// Unit test: INH timing after wake-up event
TEST_F(INHControllerTest, INHTimingAfterWakeUp) {
    uint64_t current_time = 0;
    
    // Trigger wake-up event in Standby mode
    inh_controller_update(&controller, MODE_STANDBY, false, true, current_time);
    
    EXPECT_TRUE(controller.pending_inh_assertion);
    EXPECT_FALSE(controller.inh_output_high);  // Not asserted yet
    
    // Advance time by less than tINH_SLP_STB (100μs = 100000ns)
    current_time += 50000;  // 50μs
    inh_controller_update(&controller, MODE_STANDBY, false, false, current_time);
    
    EXPECT_TRUE(controller.pending_inh_assertion);
    EXPECT_FALSE(controller.inh_output_high);  // Still not asserted
    
    // Advance time past tINH_SLP_STB
    current_time += 60000;  // Total 110μs
    inh_controller_update(&controller, MODE_STANDBY, false, false, current_time);
    
    EXPECT_FALSE(controller.pending_inh_assertion);
    EXPECT_TRUE(controller.inh_output_high);  // Now asserted
}

// Unit test: INH_MASK can override during wake-up timing
TEST_F(INHControllerTest, INHMaskOverridesWakeUpTiming) {
    uint64_t current_time = 0;
    
    // Trigger wake-up event
    inh_controller_update(&controller, MODE_STANDBY, false, true, current_time);
    
    EXPECT_TRUE(controller.pending_inh_assertion);
    
    // Set INH_MASK high during timing delay
    current_time += 50000;
    inh_controller_update(&controller, MODE_STANDBY, true, false, current_time);
    
    EXPECT_FALSE(controller.inh_enabled);
    EXPECT_FALSE(controller.inh_output_high);
    EXPECT_FALSE(controller.pending_inh_assertion);  // Cleared by mask
}

// Property test: INH output state consistency with mode
// **Validates: Requirements 7.1, 7.2**
TEST(INHControllerPropertyTest, INHOutputConsistentWithMode) {
    rc::check("INH output state consistency with mode", []() {
        INHController ctrl;
        inh_controller_init(&ctrl);
        
        // Generate random mode
        auto mode = *rc::gen::element(
            MODE_NORMAL, MODE_SILENT, MODE_STANDBY,
            MODE_GO_TO_SLEEP, MODE_SLEEP, MODE_OFF
        );
        
        // Generate random INH_MASK state
        auto inh_mask_high = *rc::gen::arbitrary<bool>();
        
        // Generate random time
        auto current_time = *rc::gen::inRange<uint64_t>(0, 1000000000);
        
        // Update controller
        inh_controller_update(&ctrl, mode, inh_mask_high, false, current_time);
        
        // Check consistency
        if (inh_mask_high) {
            // INH should be disabled
            RC_ASSERT(!ctrl.inh_enabled);
            RC_ASSERT(!ctrl.inh_output_high);
        } else {
            // INH should be enabled
            RC_ASSERT(ctrl.inh_enabled);
            
            // Check output based on mode
            if (mode == MODE_NORMAL || mode == MODE_SILENT || mode == MODE_STANDBY) {
                RC_ASSERT(ctrl.inh_output_high);
            } else {
                RC_ASSERT(!ctrl.inh_output_high);
            }
        }
    });
}

// Property test: INH_MASK functionality
// **Validates: Requirements 7.3, 7.4**
TEST(INHControllerPropertyTest, INHMaskFunctionality) {
    rc::check("INH_MASK functionality", []() {
        INHController ctrl;
        inh_controller_init(&ctrl);
        
        auto mode = *rc::gen::element(MODE_NORMAL, MODE_SILENT, MODE_STANDBY);
        auto current_time = *rc::gen::inRange<uint64_t>(0, 1000000000);
        
        // Test with INH_MASK low (enabled)
        inh_controller_update(&ctrl, mode, false, false, current_time);
        RC_ASSERT(ctrl.inh_enabled);
        RC_ASSERT(ctrl.inh_output_high);
        
        // Test with INH_MASK high (disabled)
        inh_controller_update(&ctrl, mode, true, false, current_time);
        RC_ASSERT(!ctrl.inh_enabled);
        RC_ASSERT(!ctrl.inh_output_high);
    });
}

// Property test: INH timing control
// **Validates: Requirements 7.5**
TEST(INHControllerPropertyTest, INHTimingControl) {
    rc::check("INH timing control", []() {
        INHController ctrl;
        inh_controller_init(&ctrl);
        
        auto mode = *rc::gen::element(MODE_NORMAL, MODE_SILENT, MODE_STANDBY);
        uint64_t current_time = 0;
        
        // Trigger wake-up event
        inh_controller_update(&ctrl, mode, false, true, current_time);
        RC_ASSERT(ctrl.pending_inh_assertion);
        RC_ASSERT(!ctrl.inh_output_high);
        
        // Generate delay less than tINH_SLP_STB (100μs = 100000ns)
        auto delay = *rc::gen::inRange<uint64_t>(0, 99999);
        current_time += delay;
        inh_controller_update(&ctrl, mode, false, false, current_time);
        
        // INH should not be asserted yet
        RC_ASSERT(ctrl.pending_inh_assertion);
        RC_ASSERT(!ctrl.inh_output_high);
        
        // Advance past tINH_SLP_STB
        current_time += (100000 - delay + 1);
        inh_controller_update(&ctrl, mode, false, false, current_time);
        
        // INH should now be asserted
        RC_ASSERT(!ctrl.pending_inh_assertion);
        RC_ASSERT(ctrl.inh_output_high);
    });
}
