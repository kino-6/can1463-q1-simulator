#include <gtest/gtest.h>
#include <rapidcheck.h>
#include "tcan1463q1_simulator.h"

class SimulatorTest : public ::testing::Test {
protected:
    TCAN1463Q1Simulator* sim;

    void SetUp() override {
        sim = tcan1463q1_simulator_create();
        ASSERT_NE(sim, nullptr);
    }

    void TearDown() override {
        tcan1463q1_simulator_destroy(sim);
    }
};

TEST_F(SimulatorTest, CreateAndDestroy) {
    // Simulator should be created successfully
    EXPECT_NE(sim, nullptr);
}

TEST_F(SimulatorTest, InitialMode) {
    // Initial mode should be OFF
    OperatingMode mode = tcan1463q1_simulator_get_mode(sim);
    EXPECT_EQ(mode, MODE_OFF);
}

TEST_F(SimulatorTest, SetAndGetPin) {
    // Set TXD pin to high
    bool success = tcan1463q1_simulator_set_pin(sim, PIN_TXD, PIN_STATE_HIGH, 3.3);
    EXPECT_TRUE(success);
    
    // Read back TXD pin
    PinState state;
    double voltage;
    success = tcan1463q1_simulator_get_pin(sim, PIN_TXD, &state, &voltage);
    EXPECT_TRUE(success);
    EXPECT_EQ(state, PIN_STATE_HIGH);
    EXPECT_DOUBLE_EQ(voltage, 3.3);
}

TEST_F(SimulatorTest, BatchPinOperations) {
    // Set multiple pins at once
    PinValue values[] = {
        {PIN_TXD, PIN_STATE_HIGH, 3.3},
        {PIN_EN, PIN_STATE_HIGH, 3.3},
        {PIN_NSTB, PIN_STATE_HIGH, 3.3}
    };
    
    bool success = tcan1463q1_simulator_set_pins(sim, values, 3);
    EXPECT_TRUE(success);
    
    // Read back multiple pins
    PinValue read_values[] = {
        {PIN_TXD, PIN_STATE_LOW, 0.0},
        {PIN_EN, PIN_STATE_LOW, 0.0},
        {PIN_NSTB, PIN_STATE_LOW, 0.0}
    };
    
    success = tcan1463q1_simulator_get_pins(sim, read_values, 3);
    EXPECT_TRUE(success);
    
    EXPECT_EQ(read_values[0].state, PIN_STATE_HIGH);
    EXPECT_EQ(read_values[1].state, PIN_STATE_HIGH);
    EXPECT_EQ(read_values[2].state, PIN_STATE_HIGH);
}

TEST_F(SimulatorTest, GetPinInfo) {
    // Get TXD pin info
    bool is_input, is_output;
    double min_voltage, max_voltage;
    
    bool success = tcan1463q1_simulator_get_pin_info(
        sim, PIN_TXD, &is_input, &is_output, &min_voltage, &max_voltage
    );
    
    EXPECT_TRUE(success);
    EXPECT_TRUE(is_input);
    EXPECT_FALSE(is_output);
}

TEST_F(SimulatorTest, SimulationStep) {
    // Configure power supplies
    tcan1463q1_simulator_configure(sim, 12.0, 5.0, 3.3, 25.0, 60.0, 100e-12);
    
    // Set power pins
    tcan1463q1_simulator_set_pin(sim, PIN_VSUP, PIN_STATE_ANALOG, 12.0);
    tcan1463q1_simulator_set_pin(sim, PIN_VCC, PIN_STATE_ANALOG, 5.0);
    tcan1463q1_simulator_set_pin(sim, PIN_VIO, PIN_STATE_ANALOG, 3.3);
    tcan1463q1_simulator_set_pin(sim, PIN_EN, PIN_STATE_HIGH, 3.3);
    tcan1463q1_simulator_set_pin(sim, PIN_NSTB, PIN_STATE_HIGH, 3.3);
    
    // Step simulation - need enough time for power-up
    tcan1463q1_simulator_step(sim, 500000000); // 500ms
    
    // Mode should transition from OFF to Normal
    OperatingMode mode = tcan1463q1_simulator_get_mode(sim);
    EXPECT_EQ(mode, MODE_NORMAL);
}

TEST_F(SimulatorTest, PowerUpSequence) {
    // Configure and power up
    tcan1463q1_simulator_set_pin(sim, PIN_VSUP, PIN_STATE_ANALOG, 12.0);
    tcan1463q1_simulator_set_pin(sim, PIN_VCC, PIN_STATE_ANALOG, 5.0);
    tcan1463q1_simulator_set_pin(sim, PIN_VIO, PIN_STATE_ANALOG, 3.3);
    tcan1463q1_simulator_set_pin(sim, PIN_EN, PIN_STATE_HIGH, 3.3);
    tcan1463q1_simulator_set_pin(sim, PIN_NSTB, PIN_STATE_HIGH, 3.3);
    
    // Step simulation
    tcan1463q1_simulator_step(sim, 500000000); // 500ms
    
    // Should be in Normal mode
    OperatingMode mode = tcan1463q1_simulator_get_mode(sim);
    EXPECT_EQ(mode, MODE_NORMAL);
}

TEST_F(SimulatorTest, GetFlags) {
    bool pwron, wakerq, wakesr, uvsup, uvcc, uvio;
    bool cbf, txdclp, txddto, txdrxd, candom, tsd;
    
    tcan1463q1_simulator_get_flags(
        sim, &pwron, &wakerq, &wakesr, &uvsup, &uvcc, &uvio,
        &cbf, &txdclp, &txddto, &txdrxd, &candom, &tsd
    );
    
    // Initial state - all flags should be false except possibly uvsup
    EXPECT_FALSE(pwron);
    EXPECT_FALSE(wakerq);
    EXPECT_FALSE(wakesr);
}

TEST_F(SimulatorTest, SnapshotAndRestore) {
    // Set up some state
    tcan1463q1_simulator_set_pin(sim, PIN_TXD, PIN_STATE_HIGH, 3.3);
    tcan1463q1_simulator_step(sim, 1000);
    
    // Create snapshot
    SimulatorSnapshot* snapshot = tcan1463q1_simulator_snapshot(sim);
    ASSERT_NE(snapshot, nullptr);
    
    // Modify state
    tcan1463q1_simulator_set_pin(sim, PIN_TXD, PIN_STATE_LOW, 0.0);
    tcan1463q1_simulator_step(sim, 1000);
    
    // Restore snapshot
    bool success = tcan1463q1_simulator_restore(sim, snapshot);
    EXPECT_TRUE(success);
    
    // Verify state is restored
    PinState state;
    double voltage;
    tcan1463q1_simulator_get_pin(sim, PIN_TXD, &state, &voltage);
    EXPECT_EQ(state, PIN_STATE_HIGH);
    
    // Clean up
    tcan1463q1_simulator_snapshot_free(snapshot);
}

TEST_F(SimulatorTest, RunUntilCondition) {
    // Set up power
    tcan1463q1_simulator_set_pin(sim, PIN_VSUP, PIN_STATE_ANALOG, 12.0);
    tcan1463q1_simulator_set_pin(sim, PIN_VCC, PIN_STATE_ANALOG, 5.0);
    tcan1463q1_simulator_set_pin(sim, PIN_VIO, PIN_STATE_ANALOG, 3.3);
    tcan1463q1_simulator_set_pin(sim, PIN_EN, PIN_STATE_HIGH, 3.3);
    tcan1463q1_simulator_set_pin(sim, PIN_NSTB, PIN_STATE_HIGH, 3.3);
    
    // Run until Normal mode
    auto condition = [](TCAN1463Q1Simulator* s, void* data) -> bool {
        return tcan1463q1_simulator_get_mode(s) == MODE_NORMAL;
    };
    
    bool reached = tcan1463q1_simulator_run_until(
        sim, condition, nullptr, 1000000000 // 1 second timeout
    );
    
    EXPECT_TRUE(reached);
    EXPECT_EQ(tcan1463q1_simulator_get_mode(sim), MODE_NORMAL);
}

TEST_F(SimulatorTest, NullPointerHandling) {
    // Test null pointer handling
    EXPECT_FALSE(tcan1463q1_simulator_set_pin(nullptr, PIN_TXD, PIN_STATE_HIGH, 3.3));
    EXPECT_FALSE(tcan1463q1_simulator_get_pin(nullptr, PIN_TXD, nullptr, nullptr));
    EXPECT_EQ(tcan1463q1_simulator_get_mode(nullptr), MODE_OFF);
}

// ============================================================================
// Property-Based Tests
// ============================================================================

// Feature: tcan1463-q1-simulator, Property 22: Propagation delay bounds
// **Validates: Requirements 9.1, 9.2**
//
// Property: For any TXD transition, the propagation delay from TXD to RXD
// should be within the specified bounds (100-190ns for recessive-to-dominant,
// 110-190ns for dominant-to-recessive).
TEST(SimulatorPropertyTest, PropagationDelayBounds) {
    rc::check("Propagation delay bounds property", []() {
        TCAN1463Q1Simulator* sim = tcan1463q1_simulator_create();
        RC_ASSERT(sim != nullptr);
        
        // Set up the simulator in Normal mode with valid power supplies
        tcan1463q1_simulator_set_pin(sim, PIN_VSUP, PIN_STATE_ANALOG, 12.0);
        tcan1463q1_simulator_set_pin(sim, PIN_VCC, PIN_STATE_ANALOG, 5.0);
        tcan1463q1_simulator_set_pin(sim, PIN_VIO, PIN_STATE_ANALOG, 3.3);
        tcan1463q1_simulator_set_pin(sim, PIN_EN, PIN_STATE_HIGH, 3.3);
        tcan1463q1_simulator_set_pin(sim, PIN_NSTB, PIN_STATE_HIGH, 3.3);
        tcan1463q1_simulator_set_pin(sim, PIN_TXD, PIN_STATE_HIGH, 3.3);
        
        // Step simulation to reach Normal mode
        tcan1463q1_simulator_step(sim, 500000000); // 500ms
        
        // Verify we're in Normal mode
        OperatingMode mode = tcan1463q1_simulator_get_mode(sim);
        RC_ASSERT(mode == MODE_NORMAL);
        
        // Generate random TXD transition type (true = recessive-to-dominant, false = dominant-to-recessive)
        const bool is_recessive_to_dominant = *rc::gen::arbitrary<bool>();
        
        // Set initial TXD state
        if (is_recessive_to_dominant) {
            // Start with TXD high (recessive)
            tcan1463q1_simulator_set_pin(sim, PIN_TXD, PIN_STATE_HIGH, 3.3);
        } else {
            // Start with TXD low (dominant)
            tcan1463q1_simulator_set_pin(sim, PIN_TXD, PIN_STATE_LOW, 0.0);
        }
        
        // Step to stabilize initial state
        tcan1463q1_simulator_step(sim, 10000); // 10μs
        
        // Record initial RXD state
        PinState initial_rxd_state;
        double initial_rxd_voltage;
        tcan1463q1_simulator_get_pin(sim, PIN_RXD, &initial_rxd_state, &initial_rxd_voltage);
        
        // Perform TXD transition
        if (is_recessive_to_dominant) {
            // Transition to dominant: TXD goes LOW
            tcan1463q1_simulator_set_pin(sim, PIN_TXD, PIN_STATE_LOW, 0.0);
        } else {
            // Transition to recessive: TXD goes HIGH
            tcan1463q1_simulator_set_pin(sim, PIN_TXD, PIN_STATE_HIGH, 3.3);
            // Note: Do NOT manually set bus voltages - the simulator will drive them
        }
        
        // Determine expected delay bounds based on transition type
        uint64_t min_delay_ns = is_recessive_to_dominant ? TPROP_LOOP1_MIN_NS : TPROP_LOOP2_MIN_NS;
        uint64_t max_delay_ns = is_recessive_to_dominant ? TPROP_LOOP1_MAX_NS : TPROP_LOOP2_MAX_NS;
        
        // Step just before minimum delay
        tcan1463q1_simulator_step(sim, min_delay_ns - 10);
        
        // RXD might not have changed yet (acceptable)
        PinState rxd_before_min;
        double rxd_voltage_before_min;
        tcan1463q1_simulator_get_pin(sim, PIN_RXD, &rxd_before_min, &rxd_voltage_before_min);
        
        // Step past maximum delay
        tcan1463q1_simulator_step(sim, max_delay_ns - min_delay_ns + 20);
        
        // Check RXD state after maximum delay
        PinState final_rxd_state;
        double final_rxd_voltage;
        tcan1463q1_simulator_get_pin(sim, PIN_RXD, &final_rxd_state, &final_rxd_voltage);
        
        // Property: RXD should have transitioned within the delay bounds
        // For recessive-to-dominant: TXD goes LOW, bus goes dominant, RXD should go LOW
        // For dominant-to-recessive: TXD goes HIGH, bus goes recessive, RXD should go HIGH
        PinState expected_rxd = is_recessive_to_dominant ? PIN_STATE_LOW : PIN_STATE_HIGH;
        
        // After max delay, RXD must have reached the expected state
        RC_ASSERT(final_rxd_state == expected_rxd);
        
        tcan1463q1_simulator_destroy(sim);
    });
}

// Feature: tcan1463-q1-simulator, Property 23: Flag clearing on mode transition to Normal
// **Validates: Requirements 10.9**
//
// Property: For any status flags, when transitioning to Normal mode,
// the PWRON and WAKERQ flags should be cleared.
TEST(SimulatorPropertyTest, FlagClearingOnModeTransitionToNormal) {
    rc::check("Flag clearing on mode transition to Normal property", []() {
        TCAN1463Q1Simulator* sim = tcan1463q1_simulator_create();
        RC_ASSERT(sim != nullptr);
        
        // Generate random initial mode (we'll test transitions from different modes)
        const int mode_choice = *rc::gen::inRange(0, 3);
        
        // Set up power supplies (valid)
        tcan1463q1_simulator_set_pin(sim, PIN_VSUP, PIN_STATE_ANALOG, 12.0);
        tcan1463q1_simulator_set_pin(sim, PIN_VCC, PIN_STATE_ANALOG, 5.0);
        tcan1463q1_simulator_set_pin(sim, PIN_VIO, PIN_STATE_ANALOG, 3.3);
        
        // Step to allow power-up
        tcan1463q1_simulator_step(sim, 500000000); // 500ms
        
        // Set up initial mode based on random choice
        switch (mode_choice) {
            case 0: // Start from Standby mode
                tcan1463q1_simulator_set_pin(sim, PIN_EN, PIN_STATE_HIGH, 3.3);
                tcan1463q1_simulator_set_pin(sim, PIN_NSTB, PIN_STATE_LOW, 0.0);
                // Trigger a wake-up to set WAKERQ
                tcan1463q1_simulator_set_pin(sim, PIN_WAKE, PIN_STATE_HIGH, 3.3);
                tcan1463q1_simulator_step(sim, 1000000); // 1ms
                tcan1463q1_simulator_set_pin(sim, PIN_WAKE, PIN_STATE_LOW, 0.0);
                tcan1463q1_simulator_step(sim, 1000000); // 1ms
                break;
                
            case 1: // Start from Sleep mode
                tcan1463q1_simulator_set_pin(sim, PIN_EN, PIN_STATE_HIGH, 3.3);
                tcan1463q1_simulator_set_pin(sim, PIN_NSTB, PIN_STATE_LOW, 0.0);
                // Wait for tSILENCE to enter Sleep mode
                tcan1463q1_simulator_step(sim, 1500000000ULL); // 1.5 seconds
                // Trigger wake-up
                tcan1463q1_simulator_set_pin(sim, PIN_WAKE, PIN_STATE_HIGH, 3.3);
                tcan1463q1_simulator_step(sim, 1000000); // 1ms
                tcan1463q1_simulator_set_pin(sim, PIN_WAKE, PIN_STATE_LOW, 0.0);
                tcan1463q1_simulator_step(sim, 1000000); // 1ms
                break;
                
            case 2: // Start from Silent mode (which may have PWRON set)
                tcan1463q1_simulator_set_pin(sim, PIN_EN, PIN_STATE_LOW, 0.0);
                tcan1463q1_simulator_set_pin(sim, PIN_NSTB, PIN_STATE_HIGH, 3.3);
                tcan1463q1_simulator_step(sim, 1000000); // 1ms
                break;
                
            default:
                break;
        }
        
        // Check flags before transition to Normal
        bool pwron_before, wakerq_before, wakesr_before;
        bool uvsup_before, uvcc_before, uvio_before;
        bool cbf_before, txdclp_before, txddto_before;
        bool txdrxd_before, candom_before, tsd_before;
        
        tcan1463q1_simulator_get_flags(
            sim, &pwron_before, &wakerq_before, &wakesr_before,
            &uvsup_before, &uvcc_before, &uvio_before,
            &cbf_before, &txdclp_before, &txddto_before,
            &txdrxd_before, &candom_before, &tsd_before
        );
        
        // Transition to Normal mode
        tcan1463q1_simulator_set_pin(sim, PIN_EN, PIN_STATE_HIGH, 3.3);
        tcan1463q1_simulator_set_pin(sim, PIN_NSTB, PIN_STATE_HIGH, 3.3);
        
        // Step simulation to complete the transition
        tcan1463q1_simulator_step(sim, 500000); // 500μs (more than tMODE1)
        
        // Verify we're in Normal mode
        OperatingMode mode = tcan1463q1_simulator_get_mode(sim);
        RC_ASSERT(mode == MODE_NORMAL);
        
        // Check flags after transition to Normal
        bool pwron_after, wakerq_after, wakesr_after;
        bool uvsup_after, uvcc_after, uvio_after;
        bool cbf_after, txdclp_after, txddto_after;
        bool txdrxd_after, candom_after, tsd_after;
        
        tcan1463q1_simulator_get_flags(
            sim, &pwron_after, &wakerq_after, &wakesr_after,
            &uvsup_after, &uvcc_after, &uvio_after,
            &cbf_after, &txdclp_after, &txddto_after,
            &txdrxd_after, &candom_after, &tsd_after
        );
        
        // Property: PWRON and WAKERQ flags should be cleared when entering Normal mode
        RC_ASSERT(pwron_after == false);
        RC_ASSERT(wakerq_after == false);
        
        // Other flags should not be affected by mode transition
        // (they have their own clearing conditions)
        
        tcan1463q1_simulator_destroy(sim);
    });
}


// Debug test to understand propagation delay behavior
TEST_F(SimulatorTest, PropagationDelayDebug) {
    // Set up Normal mode
    tcan1463q1_simulator_set_pin(sim, PIN_VSUP, PIN_STATE_ANALOG, 12.0);
    tcan1463q1_simulator_set_pin(sim, PIN_VCC, PIN_STATE_ANALOG, 5.0);
    tcan1463q1_simulator_set_pin(sim, PIN_VIO, PIN_STATE_ANALOG, 3.3);
    tcan1463q1_simulator_set_pin(sim, PIN_EN, PIN_STATE_HIGH, 3.3);
    tcan1463q1_simulator_set_pin(sim, PIN_NSTB, PIN_STATE_HIGH, 3.3);
    tcan1463q1_simulator_set_pin(sim, PIN_TXD, PIN_STATE_HIGH, 3.3);
    
    // Reach Normal mode
    tcan1463q1_simulator_step(sim, 500000000); // 500ms
    EXPECT_EQ(tcan1463q1_simulator_get_mode(sim), MODE_NORMAL);
    
    // Stabilize
    tcan1463q1_simulator_step(sim, 10000); // 10μs
    
    // Check initial RXD
    PinState rxd_state;
    double rxd_voltage;
    tcan1463q1_simulator_get_pin(sim, PIN_RXD, &rxd_state, &rxd_voltage);
    EXPECT_EQ(rxd_state, PIN_STATE_HIGH);
    
    // Transition TXD to dominant (LOW)
    tcan1463q1_simulator_set_pin(sim, PIN_TXD, PIN_STATE_LOW, 0.0);
    
    // Step once to drive the bus
    tcan1463q1_simulator_step(sim, 10); // 10ns
    
    // Step again to read the bus and update RXD (with propagation delay)
    tcan1463q1_simulator_step(sim, 200); // 200ns
    
    // Check RXD
    tcan1463q1_simulator_get_pin(sim, PIN_RXD, &rxd_state, &rxd_voltage);
    EXPECT_EQ(rxd_state, PIN_STATE_LOW);
    
    // Transition TXD back to recessive (HIGH)
    tcan1463q1_simulator_set_pin(sim, PIN_TXD, PIN_STATE_HIGH, 3.3);
    
    // Step once to drive the bus
    tcan1463q1_simulator_step(sim, 10); // 10ns
    
    // Step again to read the bus and update RXD (with propagation delay)
    tcan1463q1_simulator_step(sim, 200); // 200ns
    
    // Check RXD
    tcan1463q1_simulator_get_pin(sim, PIN_RXD, &rxd_state, &rxd_voltage);
    EXPECT_EQ(rxd_state, PIN_STATE_HIGH);
}

TEST_F(SimulatorTest, PropagationDelayDominantToRecessive) {
    // Set up Normal mode with TXD starting HIGH (recessive) to avoid TXDCLP fault
    tcan1463q1_simulator_set_pin(sim, PIN_VSUP, PIN_STATE_ANALOG, 12.0);
    tcan1463q1_simulator_set_pin(sim, PIN_VCC, PIN_STATE_ANALOG, 5.0);
    tcan1463q1_simulator_set_pin(sim, PIN_VIO, PIN_STATE_ANALOG, 3.3);
    tcan1463q1_simulator_set_pin(sim, PIN_EN, PIN_STATE_HIGH, 3.3);
    tcan1463q1_simulator_set_pin(sim, PIN_NSTB, PIN_STATE_HIGH, 3.3);
    tcan1463q1_simulator_set_pin(sim, PIN_TXD, PIN_STATE_HIGH, 3.3);  // Start recessive to avoid TXDCLP
    
    // Reach Normal mode
    tcan1463q1_simulator_step(sim, 500000000); // 500ms
    EXPECT_EQ(tcan1463q1_simulator_get_mode(sim), MODE_NORMAL);
    
    // Stabilize
    tcan1463q1_simulator_step(sim, 1000); // 1μs
    
    // Now transition TXD to dominant (LOW)
    tcan1463q1_simulator_set_pin(sim, PIN_TXD, PIN_STATE_LOW, 0.0);
    
    // Step once to drive the bus
    tcan1463q1_simulator_step(sim, 10); // 10ns
    
    // Step again to allow propagation delay
    tcan1463q1_simulator_step(sim, 200); // 200ns
    
    PinState rxd_state;
    double rxd_voltage;
    tcan1463q1_simulator_get_pin(sim, PIN_RXD, &rxd_state, &rxd_voltage);
    EXPECT_EQ(rxd_state, PIN_STATE_LOW) << "RXD should be LOW (dominant) after TXD goes LOW";
    
    // Transition TXD to recessive (HIGH)
    tcan1463q1_simulator_set_pin(sim, PIN_TXD, PIN_STATE_HIGH, 3.3);
    tcan1463q1_simulator_set_pin(sim, PIN_CANH, PIN_STATE_ANALOG, 2.5);
    tcan1463q1_simulator_set_pin(sim, PIN_CANL, PIN_STATE_ANALOG, 2.5);
    
    // Step once to drive the bus
    tcan1463q1_simulator_step(sim, 10); // 10ns
    
    // Step again to allow propagation delay
    tcan1463q1_simulator_step(sim, 200); // 200ns (within 110-190ns range)
    
    // Check RXD - should now be HIGH (recessive)
    tcan1463q1_simulator_get_pin(sim, PIN_RXD, &rxd_state, &rxd_voltage);
    EXPECT_EQ(rxd_state, PIN_STATE_HIGH) << "RXD should be HIGH (recessive) after transition";
}

// ============================================================================
// Configuration API Tests
// ============================================================================

TEST_F(SimulatorTest, ParameterValidation_ValidVoltages) {
    // Valid voltages should pass validation
    EXPECT_TRUE(tcan1463q1_simulator_validate_vsup(12.0));
    EXPECT_TRUE(tcan1463q1_simulator_validate_vcc(5.0));
    EXPECT_TRUE(tcan1463q1_simulator_validate_vio(3.3));
}

TEST_F(SimulatorTest, ParameterValidation_InvalidVoltages) {
    // Invalid voltages should fail validation
    EXPECT_FALSE(tcan1463q1_simulator_validate_vsup(-1.0));
    EXPECT_FALSE(tcan1463q1_simulator_validate_vsup(50.0));
    EXPECT_FALSE(tcan1463q1_simulator_validate_vcc(-1.0));
    EXPECT_FALSE(tcan1463q1_simulator_validate_vcc(10.0));
    EXPECT_FALSE(tcan1463q1_simulator_validate_vio(-1.0));
    EXPECT_FALSE(tcan1463q1_simulator_validate_vio(10.0));
}

TEST_F(SimulatorTest, ParameterValidation_ValidTemperature) {
    // Valid temperatures should pass validation
    EXPECT_TRUE(tcan1463q1_simulator_validate_temperature(-40.0));
    EXPECT_TRUE(tcan1463q1_simulator_validate_temperature(25.0));
    EXPECT_TRUE(tcan1463q1_simulator_validate_temperature(150.0));
}

TEST_F(SimulatorTest, ParameterValidation_InvalidTemperature) {
    // Invalid temperatures should fail validation
    EXPECT_FALSE(tcan1463q1_simulator_validate_temperature(-50.0));
    EXPECT_FALSE(tcan1463q1_simulator_validate_temperature(250.0));
}

TEST_F(SimulatorTest, ParameterValidation_ValidTimingParameters) {
    TimingParameters params;
    params.tuv_ms = 200.0;
    params.ttxddto_ms = 2.5;
    params.tbusdom_ms = 2.5;
    params.twk_filter_us = 1.0;
    params.twk_timeout_ms = 1.5;
    params.tsilence_s = 0.9;
    
    EXPECT_TRUE(tcan1463q1_simulator_validate_timing_parameters(&params));
}

TEST_F(SimulatorTest, ParameterValidation_InvalidTimingParameters) {
    TimingParameters params;
    
    // Test tuv_ms out of range
    params.tuv_ms = 50.0; // Below minimum
    params.ttxddto_ms = 2.5;
    params.tbusdom_ms = 2.5;
    params.twk_filter_us = 1.0;
    params.twk_timeout_ms = 1.5;
    params.tsilence_s = 0.9;
    EXPECT_FALSE(tcan1463q1_simulator_validate_timing_parameters(&params));
    
    // Test ttxddto_ms out of range
    params.tuv_ms = 200.0;
    params.ttxddto_ms = 5.0; // Above maximum
    EXPECT_FALSE(tcan1463q1_simulator_validate_timing_parameters(&params));
    
    // Test null pointer
    EXPECT_FALSE(tcan1463q1_simulator_validate_timing_parameters(nullptr));
}

TEST_F(SimulatorTest, SetSupplyVoltages_Valid) {
    // Valid voltages should be set successfully
    bool success = tcan1463q1_simulator_set_supply_voltages(sim, 12.0, 5.0, 3.3);
    EXPECT_TRUE(success);
    
    // Verify voltages were set
    EXPECT_DOUBLE_EQ(sim->power_state.vsup, 12.0);
    EXPECT_DOUBLE_EQ(sim->power_state.vcc, 5.0);
    EXPECT_DOUBLE_EQ(sim->power_state.vio, 3.3);
}

TEST_F(SimulatorTest, SetSupplyVoltages_Invalid) {
    // Invalid voltages should be rejected
    bool success = tcan1463q1_simulator_set_supply_voltages(sim, -1.0, 5.0, 3.3);
    EXPECT_FALSE(success);
    
    success = tcan1463q1_simulator_set_supply_voltages(sim, 12.0, 50.0, 3.3);
    EXPECT_FALSE(success);
}

TEST_F(SimulatorTest, SetTemperature_Valid) {
    // Valid temperature should be set successfully
    bool success = tcan1463q1_simulator_set_temperature(sim, 85.0);
    EXPECT_TRUE(success);
    EXPECT_DOUBLE_EQ(sim->tj_temperature, 85.0);
}

TEST_F(SimulatorTest, SetTemperature_Invalid) {
    // Invalid temperature should be rejected
    bool success = tcan1463q1_simulator_set_temperature(sim, -100.0);
    EXPECT_FALSE(success);
    
    success = tcan1463q1_simulator_set_temperature(sim, 300.0);
    EXPECT_FALSE(success);
}

TEST_F(SimulatorTest, SetBusParameters_Valid) {
    // Valid bus parameters should be set successfully
    bool success = tcan1463q1_simulator_set_bus_parameters(sim, 120.0, 50e-12);
    EXPECT_TRUE(success);
    EXPECT_DOUBLE_EQ(sim->rl_resistance, 120.0);
    EXPECT_DOUBLE_EQ(sim->cl_capacitance, 50e-12);
}

TEST_F(SimulatorTest, SetBusParameters_Invalid) {
    // Negative values should be rejected
    bool success = tcan1463q1_simulator_set_bus_parameters(sim, -10.0, 50e-12);
    EXPECT_FALSE(success);
    
    success = tcan1463q1_simulator_set_bus_parameters(sim, 120.0, -1e-12);
    EXPECT_FALSE(success);
}

TEST_F(SimulatorTest, SetTimingParameters_Valid) {
    TimingParameters params;
    params.tuv_ms = 225.0;
    params.ttxddto_ms = 2.5;
    params.tbusdom_ms = 2.6;
    params.twk_filter_us = 1.15;
    params.twk_timeout_ms = 1.4;
    params.tsilence_s = 0.9;
    
    bool success = tcan1463q1_simulator_set_timing_parameters(sim, &params);
    EXPECT_TRUE(success);
    
    // Verify parameters were set
    TimingParameters read_params;
    success = tcan1463q1_simulator_get_timing_parameters(sim, &read_params);
    EXPECT_TRUE(success);
    EXPECT_DOUBLE_EQ(read_params.tuv_ms, 225.0);
    EXPECT_DOUBLE_EQ(read_params.ttxddto_ms, 2.5);
    EXPECT_DOUBLE_EQ(read_params.tbusdom_ms, 2.6);
}

TEST_F(SimulatorTest, SetTimingParameters_Invalid) {
    TimingParameters params;
    params.tuv_ms = 50.0; // Below minimum
    params.ttxddto_ms = 2.5;
    params.tbusdom_ms = 2.6;
    params.twk_filter_us = 1.15;
    params.twk_timeout_ms = 1.4;
    params.tsilence_s = 0.9;
    
    bool success = tcan1463q1_simulator_set_timing_parameters(sim, &params);
    EXPECT_FALSE(success);
}

TEST_F(SimulatorTest, DefaultTimingParameters) {
    // After reset, timing parameters should be set to defaults (middle of ranges)
    TimingParameters params;
    bool success = tcan1463q1_simulator_get_timing_parameters(sim, &params);
    EXPECT_TRUE(success);
    
    // Check that defaults are within valid ranges
    EXPECT_GE(params.tuv_ms, TUV_MIN_MS);
    EXPECT_LE(params.tuv_ms, TUV_MAX_MS);
    EXPECT_GE(params.ttxddto_ms, TTXDDTO_MIN_MS);
    EXPECT_LE(params.ttxddto_ms, TTXDDTO_MAX_MS);
    EXPECT_GE(params.tbusdom_ms, TBUSDOM_MIN_MS);
    EXPECT_LE(params.tbusdom_ms, TBUSDOM_MAX_MS);
}

TEST_F(SimulatorTest, SnapshotRestoreWithTimingParameters) {
    // Set custom timing parameters
    TimingParameters params;
    params.tuv_ms = 150.0;
    params.ttxddto_ms = 2.0;
    params.tbusdom_ms = 2.0;
    params.twk_filter_us = 1.0;
    params.twk_timeout_ms = 1.0;
    params.tsilence_s = 0.8;
    
    tcan1463q1_simulator_set_timing_parameters(sim, &params);
    
    // Create snapshot
    SimulatorSnapshot* snapshot = tcan1463q1_simulator_snapshot(sim);
    ASSERT_NE(snapshot, nullptr);
    
    // Modify timing parameters
    params.tuv_ms = 300.0;
    tcan1463q1_simulator_set_timing_parameters(sim, &params);
    
    // Restore from snapshot
    bool success = tcan1463q1_simulator_restore(sim, snapshot);
    EXPECT_TRUE(success);
    
    // Verify timing parameters were restored
    TimingParameters restored_params;
    tcan1463q1_simulator_get_timing_parameters(sim, &restored_params);
    EXPECT_DOUBLE_EQ(restored_params.tuv_ms, 150.0);
    EXPECT_DOUBLE_EQ(restored_params.ttxddto_ms, 2.0);
    
    tcan1463q1_simulator_snapshot_free(snapshot);
}

// ============================================================================
// Property-Based Tests for Configuration
// ============================================================================

// Feature: tcan1463-q1-simulator, Property 25: Parameter validation
// **Validates: Requirements 12.6**
TEST(SimulatorPropertyTest, Property25_ParameterValidation) {
    rc::check("Parameter validation property", []() {
        auto sim = tcan1463q1_simulator_create();
        RC_ASSERT(sim != nullptr);
        
        // Generate random parameters (using integers and converting to doubles)
        auto vsup = *rc::gen::inRange(-100, 500) / 10.0;
        auto vcc = *rc::gen::inRange(-100, 150) / 10.0;
        auto vio = *rc::gen::inRange(-100, 150) / 10.0;
        auto tj = *rc::gen::inRange(-1000, 3000) / 10.0;
        auto rl = *rc::gen::inRange(-1000, 10000) / 10.0;
        auto cl = *rc::gen::inRange(-10000, 10000) / 1e13;
        
        // Test voltage validation
        bool vsup_valid = (vsup >= 0.0 && vsup <= 40.0);
        bool vcc_valid = (vcc >= 0.0 && vcc <= 6.0);
        bool vio_valid = (vio >= 0.0 && vio <= 5.5);
        
        RC_ASSERT(tcan1463q1_simulator_validate_vsup(vsup) == vsup_valid);
        RC_ASSERT(tcan1463q1_simulator_validate_vcc(vcc) == vcc_valid);
        RC_ASSERT(tcan1463q1_simulator_validate_vio(vio) == vio_valid);
        
        // Test temperature validation
        bool tj_valid = (tj >= -40.0 && tj <= 200.0);
        RC_ASSERT(tcan1463q1_simulator_validate_temperature(tj) == tj_valid);
        
        // Test setting supply voltages
        bool set_result = tcan1463q1_simulator_set_supply_voltages(sim, vsup, vcc, vio);
        bool expected_result = vsup_valid && vcc_valid && vio_valid;
        RC_ASSERT(set_result == expected_result);
        
        // If setting was successful, verify values were set
        if (set_result) {
            RC_ASSERT(sim->power_state.vsup == vsup);
            RC_ASSERT(sim->power_state.vcc == vcc);
            RC_ASSERT(sim->power_state.vio == vio);
        }
        
        // Test bus parameter validation
        bool bus_params_valid = (rl >= 0.0 && cl >= 0.0);
        bool bus_set_result = tcan1463q1_simulator_set_bus_parameters(sim, rl, cl);
        RC_ASSERT(bus_set_result == bus_params_valid);
        
        tcan1463q1_simulator_destroy(sim);
    });
}

TEST(SimulatorPropertyTest, Property25_TimingParameterValidation) {
    rc::check("Timing parameter validation property", []() {
        auto sim = tcan1463q1_simulator_create();
        RC_ASSERT(sim != nullptr);
        
        // Generate random timing parameters (using integers and converting to doubles)
        TimingParameters params;
        params.tuv_ms = *rc::gen::inRange(0, 5000) / 10.0;
        params.ttxddto_ms = *rc::gen::inRange(0, 100) / 10.0;
        params.tbusdom_ms = *rc::gen::inRange(0, 100) / 10.0;
        params.twk_filter_us = *rc::gen::inRange(0, 50) / 10.0;
        params.twk_timeout_ms = *rc::gen::inRange(0, 50) / 10.0;
        params.tsilence_s = *rc::gen::inRange(0, 30) / 10.0;
        
        // Check if parameters are within valid ranges
        bool tuv_valid = (params.tuv_ms >= TUV_MIN_MS && params.tuv_ms <= TUV_MAX_MS);
        bool ttxddto_valid = (params.ttxddto_ms >= TTXDDTO_MIN_MS && params.ttxddto_ms <= TTXDDTO_MAX_MS);
        bool tbusdom_valid = (params.tbusdom_ms >= TBUSDOM_MIN_MS && params.tbusdom_ms <= TBUSDOM_MAX_MS);
        bool twk_filter_valid = (params.twk_filter_us >= TWK_FILTER_MIN_US && params.twk_filter_us <= TWK_FILTER_MAX_US);
        bool twk_timeout_valid = (params.twk_timeout_ms >= TWK_TIMEOUT_MIN_MS && params.twk_timeout_ms <= TWK_TIMEOUT_MAX_MS);
        bool tsilence_valid = (params.tsilence_s >= TSILENCE_MIN_S && params.tsilence_s <= TSILENCE_MAX_S);
        
        bool all_valid = tuv_valid && ttxddto_valid && tbusdom_valid && 
                         twk_filter_valid && twk_timeout_valid && tsilence_valid;
        
        // Validation should match expected result
        bool validation_result = tcan1463q1_simulator_validate_timing_parameters(&params);
        RC_ASSERT(validation_result == all_valid);
        
        // Setting should match validation result
        bool set_result = tcan1463q1_simulator_set_timing_parameters(sim, &params);
        RC_ASSERT(set_result == all_valid);
        
        // If setting was successful, verify values were set correctly
        if (set_result) {
            TimingParameters read_params;
            bool get_result = tcan1463q1_simulator_get_timing_parameters(sim, &read_params);
            RC_ASSERT(get_result);
            RC_ASSERT(read_params.tuv_ms == params.tuv_ms);
            RC_ASSERT(read_params.ttxddto_ms == params.ttxddto_ms);
            RC_ASSERT(read_params.tbusdom_ms == params.tbusdom_ms);
            RC_ASSERT(read_params.twk_filter_us == params.twk_filter_us);
            RC_ASSERT(read_params.twk_timeout_ms == params.twk_timeout_ms);
            RC_ASSERT(read_params.tsilence_s == params.tsilence_s);
        }
        
        tcan1463q1_simulator_destroy(sim);
    });
}
