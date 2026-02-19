#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <vector>
#include <cmath>
#include "can_transceiver.h"

class CANTransceiverTest : public ::testing::Test {
protected:
    CANTransceiver transceiver;
    
    void SetUp() override {
        can_transceiver_init(&transceiver);
    }
};

// Test initialization
TEST_F(CANTransceiverTest, Initialization) {
    EXPECT_EQ(transceiver.state, CAN_STATE_OFF);
    EXPECT_FALSE(transceiver.driver_enabled);
    EXPECT_FALSE(transceiver.receiver_enabled);
    EXPECT_TRUE(transceiver.rxd_output);  // Default high
}

// Test bus state detection from differential voltage
TEST_F(CANTransceiverTest, BusStateDetection) {
    // Requirement 3.3: CANH-CANL >= 0.9V is dominant
    EXPECT_EQ(can_transceiver_get_bus_state(0.9), BUS_STATE_DOMINANT);
    EXPECT_EQ(can_transceiver_get_bus_state(1.5), BUS_STATE_DOMINANT);
    EXPECT_EQ(can_transceiver_get_bus_state(2.0), BUS_STATE_DOMINANT);
    
    // Requirement 3.4: CANH-CANL <= 0.5V is recessive
    EXPECT_EQ(can_transceiver_get_bus_state(0.5), BUS_STATE_RECESSIVE);
    EXPECT_EQ(can_transceiver_get_bus_state(0.0), BUS_STATE_RECESSIVE);
    EXPECT_EQ(can_transceiver_get_bus_state(-0.5), BUS_STATE_RECESSIVE);
    
    // Indeterminate range
    EXPECT_EQ(can_transceiver_get_bus_state(0.6), BUS_STATE_INDETERMINATE);
    EXPECT_EQ(can_transceiver_get_bus_state(0.7), BUS_STATE_INDETERMINATE);
    EXPECT_EQ(can_transceiver_get_bus_state(0.8), BUS_STATE_INDETERMINATE);
}

// Test RXD output based on bus state
TEST_F(CANTransceiverTest, RXDOutput) {
    // Enable receiver
    transceiver.receiver_enabled = true;
    
    uint64_t current_time = 0;
    
    // Requirement 3.3: Dominant detected -> RXD low (with propagation delay)
    can_transceiver_update_rxd(&transceiver, BUS_STATE_DOMINANT, current_time, current_time);
    // RXD update is pending, advance time past propagation delay
    current_time += 200; // 200ns, past max delay
    can_transceiver_update_rxd(&transceiver, BUS_STATE_DOMINANT, current_time, current_time);
    EXPECT_FALSE(transceiver.rxd_output);
    
    // Requirement 3.4: Recessive detected -> RXD high (with propagation delay)
    can_transceiver_update_rxd(&transceiver, BUS_STATE_RECESSIVE, current_time, current_time);
    current_time += 200; // 200ns, past max delay
    can_transceiver_update_rxd(&transceiver, BUS_STATE_RECESSIVE, current_time, current_time);
    EXPECT_TRUE(transceiver.rxd_output);
    
    // Receiver disabled -> RXD high
    transceiver.receiver_enabled = false;
    can_transceiver_update_rxd(&transceiver, BUS_STATE_DOMINANT, current_time, current_time);
    EXPECT_TRUE(transceiver.rxd_output);
}

// Test driver control in Normal mode
TEST_F(CANTransceiverTest, DriverControlNormalMode) {
    transceiver.state = CAN_STATE_ACTIVE;
    transceiver.driver_enabled = true;
    
    double canh, canl;
    
    // Requirement 3.1: TXD low -> CANH high, CANL low (dominant)
    can_transceiver_drive_bus(&transceiver, true, &canh, &canl);
    EXPECT_GT(canh, canl);  // CANH > CANL for dominant
    EXPECT_NEAR(canh, 3.5, 0.1);
    EXPECT_NEAR(canl, 1.5, 0.1);
    
    // Requirement 3.2: TXD high -> recessive (high impedance)
    can_transceiver_drive_bus(&transceiver, false, &canh, &canl);
    EXPECT_NEAR(canh, 2.5, 0.1);
    EXPECT_NEAR(canl, 2.5, 0.1);
}

// Test Silent mode behavior
TEST_F(CANTransceiverTest, SilentMode) {
    // Start from OFF state, transition to AUTONOMOUS_INACTIVE
    can_transceiver_update_state_machine(
        &transceiver,
        MODE_STANDBY,
        BUS_STATE_RECESSIVE,
        true,  // vsup_valid
        1000000  // current_time
    );
    
    EXPECT_EQ(transceiver.state, CAN_STATE_AUTONOMOUS_INACTIVE);
    
    // Requirement 3.5: Silent mode disables driver but keeps receiver active
    can_transceiver_update_state_machine(
        &transceiver,
        MODE_SILENT,
        BUS_STATE_RECESSIVE,
        true,  // vsup_valid
        1000000  // current_time
    );
    
    EXPECT_EQ(transceiver.state, CAN_STATE_ACTIVE);
    EXPECT_FALSE(transceiver.driver_enabled);
    EXPECT_TRUE(transceiver.receiver_enabled);
}

// Test CAN state machine: Off to Autonomous Inactive
TEST_F(CANTransceiverTest, StateTransitionOffToAutonomousInactive) {
    EXPECT_EQ(transceiver.state, CAN_STATE_OFF);
    
    can_transceiver_update_state_machine(
        &transceiver,
        MODE_STANDBY,
        BUS_STATE_RECESSIVE,
        true,  // vsup_valid
        0
    );
    
    EXPECT_EQ(transceiver.state, CAN_STATE_AUTONOMOUS_INACTIVE);
}

// Test CAN state machine: Autonomous Inactive to Active on mode change
TEST_F(CANTransceiverTest, StateTransitionAutonomousInactiveToActive) {
    transceiver.state = CAN_STATE_AUTONOMOUS_INACTIVE;
    
    can_transceiver_update_state_machine(
        &transceiver,
        MODE_NORMAL,
        BUS_STATE_RECESSIVE,
        true,
        1000000
    );
    
    EXPECT_EQ(transceiver.state, CAN_STATE_ACTIVE);
    EXPECT_TRUE(transceiver.driver_enabled);
    EXPECT_TRUE(transceiver.receiver_enabled);
}

// Test CAN state machine: Autonomous Inactive to Autonomous Active on wake-up
TEST_F(CANTransceiverTest, StateTransitionAutonomousInactiveToActiveOnWakeup) {
    transceiver.state = CAN_STATE_AUTONOMOUS_INACTIVE;
    
    // Remote wake-up (dominant on bus)
    can_transceiver_update_state_machine(
        &transceiver,
        MODE_STANDBY,
        BUS_STATE_DOMINANT,
        true,
        1000000
    );
    
    EXPECT_EQ(transceiver.state, CAN_STATE_AUTONOMOUS_ACTIVE);
}

// Test CAN state machine: Autonomous Active to Autonomous Inactive on silence
TEST_F(CANTransceiverTest, StateTransitionAutonomousActiveToInactiveOnSilence) {
    transceiver.state = CAN_STATE_AUTONOMOUS_ACTIVE;
    uint64_t start_time = 1000000;
    
    // Update with bus activity
    can_transceiver_update_state_machine(
        &transceiver,
        MODE_STANDBY,
        BUS_STATE_DOMINANT,
        true,
        start_time
    );
    
    EXPECT_EQ(transceiver.state, CAN_STATE_AUTONOMOUS_ACTIVE);
    
    // Wait for silence timeout (> 1 second)
    uint64_t after_silence = start_time + 1500000000ULL;  // 1.5 seconds
    can_transceiver_update_state_machine(
        &transceiver,
        MODE_STANDBY,
        BUS_STATE_RECESSIVE,
        true,
        after_silence
    );
    
    EXPECT_EQ(transceiver.state, CAN_STATE_AUTONOMOUS_INACTIVE);
}

// Test CAN state machine: Active to Autonomous Active on mode exit with bus activity
TEST_F(CANTransceiverTest, StateTransitionActiveToAutonomousActiveOnModeExit) {
    transceiver.state = CAN_STATE_ACTIVE;
    uint64_t current_time = 1000000;
    
    // Bus activity
    can_transceiver_update_state_machine(
        &transceiver,
        MODE_NORMAL,
        BUS_STATE_DOMINANT,
        true,
        current_time
    );
    
    // Exit to Standby with recent bus activity
    can_transceiver_update_state_machine(
        &transceiver,
        MODE_STANDBY,
        BUS_STATE_RECESSIVE,
        true,
        current_time + 100000  // 100 microseconds later
    );
    
    EXPECT_EQ(transceiver.state, CAN_STATE_AUTONOMOUS_ACTIVE);
}

// Test CAN state machine: Active to Autonomous Inactive on mode exit without bus activity
TEST_F(CANTransceiverTest, StateTransitionActiveToAutonomousInactiveOnModeExit) {
    transceiver.state = CAN_STATE_ACTIVE;
    uint64_t start_time = 1000000;
    
    // No bus activity for a long time
    can_transceiver_update_state_machine(
        &transceiver,
        MODE_NORMAL,
        BUS_STATE_RECESSIVE,
        true,
        start_time
    );
    
    // Exit to Standby after silence timeout
    uint64_t after_silence = start_time + 2000000000ULL;  // 2 seconds
    can_transceiver_update_state_machine(
        &transceiver,
        MODE_STANDBY,
        BUS_STATE_RECESSIVE,
        true,
        after_silence
    );
    
    EXPECT_EQ(transceiver.state, CAN_STATE_AUTONOMOUS_INACTIVE);
}

// Test full update function
TEST_F(CANTransceiverTest, FullUpdateFunction) {
    // Start in autonomous inactive
    transceiver.state = CAN_STATE_AUTONOMOUS_INACTIVE;
    
    // Enter Normal mode
    can_transceiver_update(
        &transceiver,
        MODE_NORMAL,
        true,   // TXD low (dominant)
        2.5,    // CANH
        2.5,    // CANL
        1000000
    );
    
    EXPECT_EQ(transceiver.state, CAN_STATE_ACTIVE);
    EXPECT_TRUE(transceiver.driver_enabled);
    EXPECT_TRUE(transceiver.receiver_enabled);
}

// Test that driver is disabled when not in active state
TEST_F(CANTransceiverTest, DriverDisabledInNonActiveState) {
    transceiver.state = CAN_STATE_AUTONOMOUS_INACTIVE;
    
    // Update state machine to set driver_enabled correctly
    can_transceiver_update_state_machine(
        &transceiver,
        MODE_STANDBY,
        BUS_STATE_RECESSIVE,
        true,
        1000000
    );
    
    // Driver should be disabled in autonomous inactive state
    EXPECT_FALSE(transceiver.driver_enabled);
    
    double canh, canl;
    can_transceiver_drive_bus(&transceiver, true, &canh, &canl);
    
    // Even with dominant request, bus should be recessive when driver disabled
    EXPECT_NEAR(canh, 2.5, 0.1);
    EXPECT_NEAR(canl, 2.5, 0.1);
}

// Feature: tcan1463-q1-simulator, Property 4: CAN transceiver behavior consistency
// **Validates: Requirements 3.1, 3.2, 3.3, 3.4, 3.5**
//
// Property: For any operating mode and bus differential voltage, the CAN transceiver
// should behave according to the following rules:
// (1) In Silent mode, driver is disabled and receiver is enabled
// (2) When Vdiff >= 0.9V, RXD is low
// (3) When Vdiff <= 0.5V, RXD is high
// (4) In Normal mode with TXD low, CANH is high and CANL is low
TEST(CANTransceiverPropertyTest, CANTransceiverBehaviorConsistency) {
    rc::check("CAN transceiver behavior consistency property", []() {
        CANTransceiver transceiver;
        can_transceiver_init(&transceiver);
        
        // Generate random operating mode (0-5)
        const auto mode_value = *rc::gen::inRange(0, 6);
        const OperatingMode mode = (OperatingMode)mode_value;
        
        // Generate random bus differential voltage (-3.0V to 5.0V)
        const auto vdiff = *rc::gen::inRange(-300, 501) / 100.0;
        
        // Generate random TXD state
        const auto txd_low = *rc::gen::arbitrary<bool>();
        
        // Calculate CANH and CANL from vdiff (assume CANL at 2.5V baseline)
        const double canl_voltage = 2.5;
        const double canh_voltage = canl_voltage + vdiff;
        
        // Current time for state machine
        const uint64_t current_time = 1000000;
        
        // Update transceiver state
        // First, set up the transceiver in a valid state for the mode
        bool vsup_valid = (mode != MODE_OFF);
        
        // Initialize to autonomous inactive if vsup is valid
        if (vsup_valid) {
            transceiver.state = CAN_STATE_AUTONOMOUS_INACTIVE;
        }
        
        // Transition to appropriate state based on mode
        BusState bus_state = can_transceiver_get_bus_state(vdiff);
        can_transceiver_update_state_machine(
            &transceiver,
            mode,
            bus_state,
            vsup_valid,
            current_time
        );
        
        // Update the full transceiver
        can_transceiver_update(
            &transceiver,
            mode,
            txd_low,
            canh_voltage,
            canl_voltage,
            current_time
        );
        
        // Property 1: In Silent mode, driver is disabled and receiver is enabled
        if (mode == MODE_SILENT && transceiver.state == CAN_STATE_ACTIVE) {
            RC_ASSERT(!transceiver.driver_enabled);
            RC_ASSERT(transceiver.receiver_enabled);
        }
        
        // Property 2: When Vdiff >= 0.9V, RXD is low (if receiver is enabled)
        if (vdiff >= 0.9 && transceiver.receiver_enabled) {
            RC_ASSERT(!transceiver.rxd_output);  // RXD low
        }
        
        // Property 3: When Vdiff <= 0.5V, RXD is high (if receiver is enabled)
        if (vdiff <= 0.5 && transceiver.receiver_enabled) {
            RC_ASSERT(transceiver.rxd_output);  // RXD high
        }
        
        // Property 4: In Normal mode with TXD low, CANH is high and CANL is low
        if (mode == MODE_NORMAL && txd_low && transceiver.state == CAN_STATE_ACTIVE) {
            double canh_out, canl_out;
            can_transceiver_drive_bus(&transceiver, txd_low, &canh_out, &canl_out);
            
            // CANH should be higher than CANL (dominant state)
            RC_ASSERT(canh_out > canl_out);
            // Check specific voltage levels (CANH ~3.5V, CANL ~1.5V)
            RC_ASSERT(canh_out >= 3.0 && canh_out <= 4.0);
            RC_ASSERT(canl_out >= 1.0 && canl_out <= 2.0);
        }
    });
}

// Feature: tcan1463-q1-simulator, Property 5: CAN FD data rate support
// **Validates: Requirements 3.6**
//
// Property: For any data rate up to 8 Mbps, the CAN transceiver should correctly
// transmit and receive data without timing violations.
//
// Test strategy: Simulate bit sequences at various data rates up to 8 Mbps.
// At 8 Mbps, the bit time is 125 ns. We verify that:
// 1. The transceiver can respond to TXD changes within the bit time
// 2. The transceiver can detect bus state changes within the bit time
// 3. RXD output correctly reflects the bus state for rapid transitions
TEST(CANTransceiverPropertyTest, CANFDDataRateSupport) {
    rc::check("CAN FD data rate support property", []() {
        CANTransceiver transceiver;
        can_transceiver_init(&transceiver);
        
        // Generate random data rate from 1 Mbps to 8 Mbps
        const auto data_rate_mbps = *rc::gen::inRange(1, 9);
        const uint64_t bit_time_ns = 1000 / data_rate_mbps;  // Bit time in nanoseconds
        
        // Generate a random bit sequence (4-16 bits)
        const auto num_bits = *rc::gen::inRange(4, 17);
        std::vector<bool> bit_sequence;
        for (int i = 0; i < num_bits; i++) {
            bit_sequence.push_back(*rc::gen::arbitrary<bool>());
        }
        
        // Set up transceiver in Normal mode
        transceiver.state = CAN_STATE_AUTONOMOUS_INACTIVE;
        uint64_t current_time = 1000000;  // Start at 1ms
        
        // Transition to Normal mode
        can_transceiver_update_state_machine(
            &transceiver,
            MODE_NORMAL,
            BUS_STATE_RECESSIVE,
            true,  // vsup_valid
            current_time
        );
        
        RC_ASSERT(transceiver.state == CAN_STATE_ACTIVE);
        RC_ASSERT(transceiver.driver_enabled);
        RC_ASSERT(transceiver.receiver_enabled);
        
        // Simulate transmission of bit sequence
        for (size_t i = 0; i < bit_sequence.size(); i++) {
            bool txd_dominant = bit_sequence[i];  // true = dominant (0), false = recessive (1)
            
            // Update transceiver with TXD input
            double canh_voltage = 2.5;
            double canl_voltage = 2.5;
            
            can_transceiver_update(
                &transceiver,
                MODE_NORMAL,
                txd_dominant,
                canh_voltage,
                canl_voltage,
                current_time
            );
            
            // Verify driver output
            double canh_out, canl_out;
            can_transceiver_drive_bus(&transceiver, txd_dominant, &canh_out, &canl_out);
            
            if (txd_dominant) {
                // Dominant bit: CANH > CANL
                RC_ASSERT(canh_out > canl_out);
                RC_ASSERT(canh_out >= 3.0 && canh_out <= 4.0);
                RC_ASSERT(canl_out >= 1.0 && canl_out <= 2.0);
            } else {
                // Recessive bit: CANH ≈ CANL
                RC_ASSERT(std::abs(canh_out - canl_out) < 0.1);
            }
            
            // Simulate bus voltage based on driver output
            double bus_canh = canh_out;
            double bus_canl = canl_out;
            double vdiff = bus_canh - bus_canl;
            
            // Update receiver with bus voltage
            can_transceiver_update(
                &transceiver,
                MODE_NORMAL,
                txd_dominant,
                bus_canh,
                bus_canl,
                current_time
            );
            
            // Verify receiver output matches transmitted bit
            BusState expected_bus_state = can_transceiver_get_bus_state(vdiff);
            
            if (expected_bus_state == BUS_STATE_DOMINANT) {
                RC_ASSERT(!transceiver.rxd_output);  // RXD low for dominant
            } else if (expected_bus_state == BUS_STATE_RECESSIVE) {
                RC_ASSERT(transceiver.rxd_output);   // RXD high for recessive
            }
            
            // Advance time by one bit period
            current_time += bit_time_ns;
        }
        
        // Verify transceiver is still operational after high-speed sequence
        RC_ASSERT(transceiver.state == CAN_STATE_ACTIVE);
        RC_ASSERT(transceiver.driver_enabled);
        RC_ASSERT(transceiver.receiver_enabled);
    });
}
