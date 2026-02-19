#include <gtest/gtest.h>
#include <rapidcheck.h>
#include "fault_detector.h"

// Test fixture for Fault Detector tests
class FaultDetectorTest : public ::testing::Test {
protected:
    FaultState state;
    
    void SetUp() override {
        fault_detector_init(&state);
    }
};

// Test initialization
TEST_F(FaultDetectorTest, InitializationSetsDefaultValues) {
    EXPECT_FALSE(state.txdclp_flag);
    EXPECT_FALSE(state.txddto_flag);
    EXPECT_FALSE(state.txdrxd_flag);
    EXPECT_FALSE(state.candom_flag);
    EXPECT_FALSE(state.tsd_flag);
    EXPECT_FALSE(state.cbf_flag);
    EXPECT_EQ(state.txd_dominant_start, UINT64_MAX);
    EXPECT_EQ(state.bus_dominant_start, UINT64_MAX);
    EXPECT_EQ(state.cbf_transition_count, 0);
}

// Test TXDCLP detection (TXD clamped low before entering Normal mode)
TEST_F(FaultDetectorTest, TXDCLPDetection) {
    // TXD low before entering Normal mode should set TXDCLP flag
    fault_detector_check_txdclp(&state, true, MODE_NORMAL);
    EXPECT_TRUE(state.txdclp_flag);
    EXPECT_TRUE(fault_detector_should_disable_driver(&state));
}

TEST_F(FaultDetectorTest, TXDCLPNotSetWhenTXDHigh) {
    // TXD high before entering Normal mode should not set TXDCLP flag
    fault_detector_check_txdclp(&state, false, MODE_NORMAL);
    EXPECT_FALSE(state.txdclp_flag);
}

TEST_F(FaultDetectorTest, TXDCLPNotSetForOtherModes) {
    // TXD low before entering Silent mode should not set TXDCLP flag
    fault_detector_check_txdclp(&state, true, MODE_SILENT);
    EXPECT_FALSE(state.txdclp_flag);
}

// Test TXDDTO detection (TXD dominant timeout)
TEST_F(FaultDetectorTest, TXDDTODetection) {
    uint64_t current_time = 0;
    
    // TXD dominant but timeout not reached
    fault_detector_check_txddto(&state, true, current_time);
    EXPECT_FALSE(state.txddto_flag);
    
    // Advance time to just before minimum timeout (1.2ms)
    current_time += 1100000ULL;  // 1.1ms
    fault_detector_check_txddto(&state, true, current_time);
    EXPECT_FALSE(state.txddto_flag);
    
    // Advance time past minimum timeout
    current_time += 200000ULL;  // Total 1.3ms
    fault_detector_check_txddto(&state, true, current_time);
    EXPECT_TRUE(state.txddto_flag);
    EXPECT_TRUE(fault_detector_should_disable_driver(&state));
}

TEST_F(FaultDetectorTest, TXDDTOResetOnRecessive) {
    uint64_t current_time = 0;
    
    // TXD dominant for some time
    fault_detector_check_txddto(&state, true, current_time);
    current_time += 1000000ULL;  // 1ms
    fault_detector_check_txddto(&state, true, current_time);
    
    // TXD goes recessive before timeout
    current_time += 100000ULL;
    fault_detector_check_txddto(&state, false, current_time);
    EXPECT_FALSE(state.txddto_flag);
    
    // TXD dominant again - should restart timer
    current_time += 100000ULL;
    fault_detector_check_txddto(&state, true, current_time);
    current_time += 1300000ULL;  // 1.3ms from new start
    fault_detector_check_txddto(&state, true, current_time);
    EXPECT_TRUE(state.txddto_flag);
}

// Test TXDRXD detection (TXD/RXD short)
TEST_F(FaultDetectorTest, TXDRXDDetection) {
    uint64_t current_time = 0;
    
    // TXD and RXD both low (shorted)
    fault_detector_check_txdrxd(&state, true, true, current_time);
    EXPECT_FALSE(state.txdrxd_flag);
    
    // Advance time past minimum timeout
    current_time += 1300000ULL;  // 1.3ms
    fault_detector_check_txdrxd(&state, true, true, current_time);
    EXPECT_TRUE(state.txdrxd_flag);
    EXPECT_TRUE(fault_detector_should_disable_driver(&state));
}

TEST_F(FaultDetectorTest, TXDRXDDetectionBothHigh) {
    uint64_t current_time = 0;
    
    // TXD and RXD both high (also shorted)
    fault_detector_check_txdrxd(&state, false, false, current_time);
    current_time += 1300000ULL;
    fault_detector_check_txdrxd(&state, false, false, current_time);
    EXPECT_TRUE(state.txdrxd_flag);
}

TEST_F(FaultDetectorTest, TXDRXDNotDetectedWhenDifferent) {
    uint64_t current_time = 0;
    
    // TXD low, RXD high (not shorted)
    fault_detector_check_txdrxd(&state, true, false, current_time);
    current_time += 1300000ULL;
    fault_detector_check_txdrxd(&state, true, false, current_time);
    EXPECT_FALSE(state.txdrxd_flag);
}

// Test CANDOM detection (CAN bus dominant fault)
TEST_F(FaultDetectorTest, CANDOMDetection) {
    uint64_t current_time = 0;
    
    // Bus dominant but timeout not reached
    fault_detector_check_candom(&state, BUS_STATE_DOMINANT, current_time);
    EXPECT_FALSE(state.candom_flag);
    
    // Advance time past minimum timeout (1.4ms)
    current_time += 1500000ULL;  // 1.5ms
    fault_detector_check_candom(&state, BUS_STATE_DOMINANT, current_time);
    EXPECT_TRUE(state.candom_flag);
    
    // CANDOM should NOT disable driver
    EXPECT_FALSE(fault_detector_should_disable_driver(&state));
}

TEST_F(FaultDetectorTest, CANDOMResetOnRecessive) {
    uint64_t current_time = 0;
    
    // Bus dominant for some time
    fault_detector_check_candom(&state, BUS_STATE_DOMINANT, current_time);
    current_time += 1000000ULL;
    fault_detector_check_candom(&state, BUS_STATE_DOMINANT, current_time);
    
    // Bus goes recessive
    current_time += 100000ULL;
    fault_detector_check_candom(&state, BUS_STATE_RECESSIVE, current_time);
    EXPECT_FALSE(state.candom_flag);
}

// Test TSD detection (Thermal shutdown)
TEST_F(FaultDetectorTest, TSDDetection) {
    // Temperature below threshold
    fault_detector_check_tsd(&state, 160.0);
    EXPECT_FALSE(state.tsd_flag);
    
    // Temperature at threshold
    fault_detector_check_tsd(&state, 165.0);
    EXPECT_TRUE(state.tsd_flag);
    EXPECT_TRUE(fault_detector_should_disable_driver(&state));
    
    // Temperature above threshold
    fault_detector_check_tsd(&state, 170.0);
    EXPECT_TRUE(state.tsd_flag);
}

TEST_F(FaultDetectorTest, TSDRecovery) {
    // Set TSD flag
    fault_detector_check_tsd(&state, 170.0);
    EXPECT_TRUE(state.tsd_flag);
    
    // Temperature drops below threshold
    fault_detector_check_tsd(&state, 160.0);
    EXPECT_FALSE(state.tsd_flag);
}

// Test CBF detection (CAN bus fault - 4 consecutive transitions)
TEST_F(FaultDetectorTest, CBFDetection) {
    // Simulate 4 consecutive dominant-to-recessive transitions
    for (int i = 0; i < 4; i++) {
        fault_detector_check_cbf(&state, BUS_STATE_DOMINANT, MODE_NORMAL);
        fault_detector_check_cbf(&state, BUS_STATE_RECESSIVE, MODE_NORMAL);
    }
    
    EXPECT_TRUE(state.cbf_flag);
    // CBF should NOT disable driver
    EXPECT_FALSE(fault_detector_should_disable_driver(&state));
}

TEST_F(FaultDetectorTest, CBFNotSetInInactiveModes) {
    // CBF should not be tracked in non-active modes
    for (int i = 0; i < 4; i++) {
        fault_detector_check_cbf(&state, BUS_STATE_DOMINANT, MODE_STANDBY);
        fault_detector_check_cbf(&state, BUS_STATE_RECESSIVE, MODE_STANDBY);
    }
    
    EXPECT_FALSE(state.cbf_flag);
}

// Test fault_detector_update (comprehensive update)
TEST_F(FaultDetectorTest, ComprehensiveUpdate) {
    uint64_t current_time = 0;
    
    // Normal operation - no faults
    fault_detector_update(&state, false, false, BUS_STATE_RECESSIVE, 25.0, current_time, MODE_NORMAL);
    EXPECT_FALSE(fault_detector_has_any_fault(&state));
    EXPECT_FALSE(fault_detector_get_nfault_state(&state));
    
    // TXD dominant for too long
    current_time += 1300000ULL;
    fault_detector_update(&state, true, false, BUS_STATE_RECESSIVE, 25.0, current_time, MODE_NORMAL);
    EXPECT_TRUE(fault_detector_has_any_fault(&state));
    EXPECT_TRUE(fault_detector_get_nfault_state(&state));
}

// Test nFAULT state
TEST_F(FaultDetectorTest, NFaultState) {
    // No faults - nFAULT should be high (false)
    EXPECT_FALSE(fault_detector_get_nfault_state(&state));
    
    // Set a fault - nFAULT should be low (true)
    state.txdclp_flag = true;
    EXPECT_TRUE(fault_detector_get_nfault_state(&state));
    
    // Clear fault - nFAULT should be high again
    state.txdclp_flag = false;
    EXPECT_FALSE(fault_detector_get_nfault_state(&state));
}

// Test null pointer handling
TEST_F(FaultDetectorTest, NullPointerHandling) {
    fault_detector_init(nullptr);  // Should not crash
    fault_detector_update(nullptr, false, false, BUS_STATE_RECESSIVE, 25.0, 0, MODE_NORMAL);
    fault_detector_check_txdclp(nullptr, false, MODE_NORMAL);
    fault_detector_check_txddto(nullptr, false, 0);
    fault_detector_check_txdrxd(nullptr, false, false, 0);
    fault_detector_check_candom(nullptr, BUS_STATE_RECESSIVE, 0);
    fault_detector_check_tsd(nullptr, 25.0);
    fault_detector_check_cbf(nullptr, BUS_STATE_RECESSIVE, MODE_NORMAL);
    EXPECT_FALSE(fault_detector_has_any_fault(nullptr));
    EXPECT_FALSE(fault_detector_get_nfault_state(nullptr));
    EXPECT_FALSE(fault_detector_should_disable_driver(nullptr));
}

// ============================================================================
// Property-Based Tests
// ============================================================================

// Feature: tcan1463-q1-simulator, Property 9: TXD dominant timeout detection
// **Validates: Requirements 5.2**
//
// Property: For any TXD state, when TXD is held dominant (low) for t >= tTXDDTO
// (1.2-3.8ms), the TXDDTO flag should be set and the CAN driver should be disabled.
TEST(FaultDetectorPropertyTest, TXDDominantTimeoutDetection) {
    rc::check("TXD dominant timeout detection property", []() {
        FaultState state;
        fault_detector_init(&state);
        
        // Generate random dominant duration (0 to 5ms)
        const auto duration_ms = *rc::gen::inRange(0, 5001) / 1000.0;
        const uint64_t duration_ns = (uint64_t)(duration_ms * 1000000.0);
        
        // Generate random starting time
        const auto start_time = *rc::gen::inRange(0, 1000000000);
        
        // Simulate TXD being held dominant
        fault_detector_check_txddto(&state, true, start_time);
        fault_detector_check_txddto(&state, true, start_time + duration_ns);
        
        // Check if TXDDTO flag is set correctly
        if (duration_ms >= TTXDDTO_MIN_MS) {
            // Duration exceeds minimum timeout - flag should be set
            RC_ASSERT(state.txddto_flag);
            RC_ASSERT(fault_detector_should_disable_driver(&state));
        } else {
            // Duration below minimum timeout - flag should not be set
            RC_ASSERT(!state.txddto_flag);
        }
    });
}

// Feature: tcan1463-q1-simulator, Property 10: TXD/RXD short detection
// **Validates: Requirements 5.3**
//
// Property: For any TXD and RXD state, when TXD and RXD are shorted (both same value)
// for t >= tTXDDTO, the TXDRXD flag should be set and the CAN driver should be disabled.
TEST(FaultDetectorPropertyTest, TXDRXDShortDetection) {
    rc::check("TXD/RXD short detection property", []() {
        FaultState state;
        fault_detector_init(&state);
        
        // Generate random TXD and RXD states
        const auto txd_low = *rc::gen::arbitrary<bool>();
        const auto rxd_low = *rc::gen::arbitrary<bool>();
        
        // Generate random duration (0 to 5ms)
        const auto duration_ms = *rc::gen::inRange(0, 5001) / 1000.0;
        const uint64_t duration_ns = (uint64_t)(duration_ms * 1000000.0);
        
        const auto start_time = *rc::gen::inRange(0, 1000000000);
        
        // Simulate TXD and RXD states
        fault_detector_check_txdrxd(&state, txd_low, rxd_low, start_time);
        fault_detector_check_txdrxd(&state, txd_low, rxd_low, start_time + duration_ns);
        
        // Check if TXDRXD flag is set correctly
        bool shorted = (txd_low == rxd_low);
        
        if (shorted && duration_ms >= TTXDDTO_MIN_MS) {
            // TXD and RXD shorted for sufficient time - flag should be set
            RC_ASSERT(state.txdrxd_flag);
            RC_ASSERT(fault_detector_should_disable_driver(&state));
        } else {
            // Either not shorted or duration insufficient - flag should not be set
            RC_ASSERT(!state.txdrxd_flag);
        }
    });
}

// Feature: tcan1463-q1-simulator, Property 11: CAN bus dominant fault detection
// **Validates: Requirements 5.4**
//
// Property: For any bus state, when the CAN bus is dominant for t >= tBUSDOM
// (1.4-3.8ms), the CANDOM flag should be set.
TEST(FaultDetectorPropertyTest, CANBusDominantFaultDetection) {
    rc::check("CAN bus dominant fault detection property", []() {
        FaultState state;
        fault_detector_init(&state);
        
        // Generate random bus state
        const auto bus_dominant = *rc::gen::arbitrary<bool>();
        const BusState bus_state = bus_dominant ? BUS_STATE_DOMINANT : BUS_STATE_RECESSIVE;
        
        // Generate random duration (0 to 5ms)
        const auto duration_ms = *rc::gen::inRange(0, 5001) / 1000.0;
        const uint64_t duration_ns = (uint64_t)(duration_ms * 1000000.0);
        
        const auto start_time = *rc::gen::inRange(0, 1000000000);
        
        // Simulate bus state
        fault_detector_check_candom(&state, bus_state, start_time);
        fault_detector_check_candom(&state, bus_state, start_time + duration_ns);
        
        // Check if CANDOM flag is set correctly
        if (bus_dominant && duration_ms >= TBUSDOM_MIN_MS) {
            // Bus dominant for sufficient time - flag should be set
            RC_ASSERT(state.candom_flag);
            // CANDOM should NOT disable driver
            RC_ASSERT(!fault_detector_should_disable_driver(&state));
        } else {
            // Either not dominant or duration insufficient - flag should not be set
            RC_ASSERT(!state.candom_flag);
        }
    });
}

// Feature: tcan1463-q1-simulator, Property 12: Thermal shutdown detection
// **Validates: Requirements 5.5**
//
// Property: For any junction temperature, when TJ >= TSDR (165°C), the TSD flag
// should be set and the CAN driver should be disabled.
TEST(FaultDetectorPropertyTest, ThermalShutdownDetection) {
    rc::check("Thermal shutdown detection property", []() {
        FaultState state;
        fault_detector_init(&state);
        
        // Generate random temperature (-40°C to 200°C)
        const auto temperature = *rc::gen::inRange(-40, 201);
        
        // Check thermal shutdown
        fault_detector_check_tsd(&state, temperature);
        
        // Verify TSD flag and driver disable state
        if (temperature >= TSDR_CELSIUS) {
            // Temperature at or above threshold - TSD flag should be set
            RC_ASSERT(state.tsd_flag);
            RC_ASSERT(fault_detector_should_disable_driver(&state));
        } else {
            // Temperature below threshold - TSD flag should not be set
            RC_ASSERT(!state.tsd_flag);
        }
    });
}

// Feature: tcan1463-q1-simulator, Property 13: Fault indication on nFAULT
// **Validates: Requirements 5.7**
//
// Property: For any fault flag (TXDCLP, TXDDTO, TXDRXD, CANDOM, TSD, CBF),
// when any fault flag is set, nFAULT pin should be driven low.
TEST(FaultDetectorPropertyTest, FaultIndicationOnNFault) {
    rc::check("Fault indication on nFAULT property", []() {
        FaultState state;
        fault_detector_init(&state);
        
        // Generate random fault flags
        state.txdclp_flag = *rc::gen::arbitrary<bool>();
        state.txddto_flag = *rc::gen::arbitrary<bool>();
        state.txdrxd_flag = *rc::gen::arbitrary<bool>();
        state.candom_flag = *rc::gen::arbitrary<bool>();
        state.tsd_flag = *rc::gen::arbitrary<bool>();
        state.cbf_flag = *rc::gen::arbitrary<bool>();
        
        // Check nFAULT state
        bool any_fault = state.txdclp_flag || state.txddto_flag || state.txdrxd_flag ||
                         state.candom_flag || state.tsd_flag || state.cbf_flag;
        
        bool nfault_low = fault_detector_get_nfault_state(&state);
        
        // Verify nFAULT is low (true) when any fault is present
        if (any_fault) {
            RC_ASSERT(nfault_low);
        } else {
            RC_ASSERT(!nfault_low);
        }
        
        // Verify has_any_fault matches
        RC_ASSERT(fault_detector_has_any_fault(&state) == any_fault);
    });
}
