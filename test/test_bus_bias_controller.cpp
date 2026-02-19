#include <gtest/gtest.h>
#include <rapidcheck.h>
#include "bus_bias_controller.h"

// Test fixture for Bus Bias Controller tests
class BusBiasControllerTest : public ::testing::Test {
protected:
    BusBiasController controller;
    
    void SetUp() override {
        bus_bias_controller_init(&controller);
    }
};

// Unit Tests

TEST_F(BusBiasControllerTest, InitializesCorrectly) {
    EXPECT_EQ(controller.state, BIAS_STATE_OFF);
    EXPECT_EQ(controller.last_bus_activity, 0ULL);
}

TEST_F(BusBiasControllerTest, UpdatesStateBasedOnCANState) {
    uint64_t current_time = 1000000ULL;
    
    // Test OFF state
    bus_bias_controller_update(&controller, CAN_STATE_OFF, BUS_STATE_RECESSIVE, current_time);
    EXPECT_EQ(controller.state, BIAS_STATE_OFF);
    
    // Test AUTONOMOUS_INACTIVE state
    bus_bias_controller_update(&controller, CAN_STATE_AUTONOMOUS_INACTIVE, BUS_STATE_RECESSIVE, current_time);
    EXPECT_EQ(controller.state, BIAS_STATE_AUTONOMOUS_INACTIVE);
    
    // Test AUTONOMOUS_ACTIVE state
    bus_bias_controller_update(&controller, CAN_STATE_AUTONOMOUS_ACTIVE, BUS_STATE_RECESSIVE, current_time);
    EXPECT_EQ(controller.state, BIAS_STATE_AUTONOMOUS_ACTIVE);
    
    // Test ACTIVE state
    bus_bias_controller_update(&controller, CAN_STATE_ACTIVE, BUS_STATE_RECESSIVE, current_time);
    EXPECT_EQ(controller.state, BIAS_STATE_ACTIVE);
}

TEST_F(BusBiasControllerTest, BiasVoltageOff) {
    double canh, canl;
    double vcc = 5.0;
    
    controller.state = BIAS_STATE_OFF;
    bus_bias_controller_get_bias(&controller, vcc, &canh, &canl);
    
    // High impedance - represented as 0V
    EXPECT_DOUBLE_EQ(canh, 0.0);
    EXPECT_DOUBLE_EQ(canl, 0.0);
}

TEST_F(BusBiasControllerTest, BiasVoltageAutonomousInactive) {
    double canh, canl;
    double vcc = 5.0;
    
    controller.state = BIAS_STATE_AUTONOMOUS_INACTIVE;
    bus_bias_controller_get_bias(&controller, vcc, &canh, &canl);
    
    // Bias to GND
    EXPECT_DOUBLE_EQ(canh, 0.0);
    EXPECT_DOUBLE_EQ(canl, 0.0);
}

TEST_F(BusBiasControllerTest, BiasVoltageAutonomousActive) {
    double canh, canl;
    double vcc = 5.0;
    
    controller.state = BIAS_STATE_AUTONOMOUS_ACTIVE;
    bus_bias_controller_get_bias(&controller, vcc, &canh, &canl);
    
    // Bias to 2.5V
    EXPECT_DOUBLE_EQ(canh, 2.5);
    EXPECT_DOUBLE_EQ(canl, 2.5);
}

TEST_F(BusBiasControllerTest, BiasVoltageActive) {
    double canh, canl;
    double vcc = 5.0;
    
    controller.state = BIAS_STATE_ACTIVE;
    bus_bias_controller_get_bias(&controller, vcc, &canh, &canl);
    
    // Bias to VCC/2
    EXPECT_DOUBLE_EQ(canh, 2.5);
    EXPECT_DOUBLE_EQ(canl, 2.5);
}

TEST_F(BusBiasControllerTest, BiasVoltageActiveWithDifferentVCC) {
    double canh, canl;
    double vcc = 3.3;
    
    controller.state = BIAS_STATE_ACTIVE;
    bus_bias_controller_get_bias(&controller, vcc, &canh, &canl);
    
    // Bias to VCC/2
    EXPECT_DOUBLE_EQ(canh, 1.65);
    EXPECT_DOUBLE_EQ(canl, 1.65);
}

TEST_F(BusBiasControllerTest, TracksBusActivity) {
    uint64_t time1 = 1000000ULL;
    uint64_t time2 = 2000000ULL;
    
    // Initial update with recessive
    bus_bias_controller_update(&controller, CAN_STATE_ACTIVE, BUS_STATE_RECESSIVE, time1);
    EXPECT_EQ(controller.last_bus_activity, time1);
    
    // Update with dominant - should update activity time
    bus_bias_controller_update(&controller, CAN_STATE_ACTIVE, BUS_STATE_DOMINANT, time2);
    EXPECT_EQ(controller.last_bus_activity, time2);
}

TEST_F(BusBiasControllerTest, SilenceTimeoutDetection) {
    uint64_t start_time = 1000000000ULL;  // 1 second
    uint64_t silence_duration = 900000000ULL;  // 0.9 seconds (tSILENCE)
    
    // Set initial activity time
    bus_bias_controller_update(&controller, CAN_STATE_ACTIVE, BUS_STATE_DOMINANT, start_time);
    
    // Check before timeout
    uint64_t before_timeout = start_time + silence_duration - 1000000ULL;
    EXPECT_FALSE(bus_bias_controller_is_silence_timeout(&controller, before_timeout));
    
    // Check after timeout
    uint64_t after_timeout = start_time + silence_duration + 1000000ULL;
    EXPECT_TRUE(bus_bias_controller_is_silence_timeout(&controller, after_timeout));
}

TEST_F(BusBiasControllerTest, NullPointerHandling) {
    double canh, canl;
    
    // Test null controller
    bus_bias_controller_init(nullptr);
    bus_bias_controller_update(nullptr, CAN_STATE_ACTIVE, BUS_STATE_RECESSIVE, 1000ULL);
    bus_bias_controller_get_bias(nullptr, 5.0, &canh, &canl);
    EXPECT_FALSE(bus_bias_controller_is_silence_timeout(nullptr, 1000ULL));
    
    // Test null output pointers
    bus_bias_controller_get_bias(&controller, 5.0, nullptr, &canl);
    bus_bias_controller_get_bias(&controller, 5.0, &canh, nullptr);
}

// Property-Based Tests

// Feature: tcan1463-q1-simulator, Property 19: Bus bias control
// **Validates: Requirements 8.1, 8.2, 8.3, 8.4**
TEST(BusBiasControllerPropertyTest, Property19_BusBiasControl) {
    rc::check("Bus bias control property", []() {
        BusBiasController controller;
        bus_bias_controller_init(&controller);
        
        // Generate random CAN state
        auto can_state = *rc::gen::element(
            CAN_STATE_OFF,
            CAN_STATE_AUTONOMOUS_INACTIVE,
            CAN_STATE_AUTONOMOUS_ACTIVE,
            CAN_STATE_ACTIVE
        );
        
        // Generate random VCC voltage (typical range 3.0V to 5.5V)
        auto vcc = *rc::gen::inRange(3000, 5501) / 1000.0;
        
        uint64_t current_time = 1000000ULL;
        
        // Update controller state
        bus_bias_controller_update(&controller, can_state, BUS_STATE_RECESSIVE, current_time);
        
        // Get bias voltages
        double canh, canl;
        bus_bias_controller_get_bias(&controller, vcc, &canh, &canl);
        
        // Verify bias voltages match expected values for each state
        switch (can_state) {
            case CAN_STATE_OFF:
                // Requirement 8.4: High impedance in CAN off mode
                RC_ASSERT(canh == 0.0);
                RC_ASSERT(canl == 0.0);
                break;
                
            case CAN_STATE_AUTONOMOUS_INACTIVE:
                // Requirement 8.3: Bias to GND in autonomous inactive mode
                RC_ASSERT(canh == 0.0);
                RC_ASSERT(canl == 0.0);
                break;
                
            case CAN_STATE_AUTONOMOUS_ACTIVE:
                // Requirement 8.2: Bias to 2.5V in autonomous active mode
                RC_ASSERT(canh == 2.5);
                RC_ASSERT(canl == 2.5);
                break;
                
            case CAN_STATE_ACTIVE:
                // Requirement 8.1: Bias to VCC/2 in active mode
                RC_ASSERT(std::abs(canh - vcc / 2.0) < 0.001);
                RC_ASSERT(std::abs(canl - vcc / 2.0) < 0.001);
                break;
        }
    });
}

// Feature: tcan1463-q1-simulator, Property 20: Autonomous state transition on silence
// **Validates: Requirements 8.5**
TEST(BusBiasControllerPropertyTest, Property20_AutonomousStateTransitionOnSilence) {
    rc::check("Autonomous state transition on silence property", []() {
        BusBiasController controller;
        bus_bias_controller_init(&controller);
        
        // Generate random start time
        auto start_time = *rc::gen::inRange(1000000ULL, 10000000000ULL);
        
        // Generate random silence duration around tSILENCE threshold
        // tSILENCE is 0.6-1.2s, using 0.9s (900ms) as implementation value
        auto silence_ns = *rc::gen::inRange(600000000ULL, 1200000000ULL);
        
        // Set initial bus activity
        bus_bias_controller_update(&controller, CAN_STATE_AUTONOMOUS_ACTIVE, BUS_STATE_DOMINANT, start_time);
        
        // Check timeout after silence duration
        uint64_t check_time = start_time + silence_ns;
        bool is_timeout = bus_bias_controller_is_silence_timeout(&controller, check_time);
        
        // Requirement 8.5: Bus inactive for t > tSILENCE should trigger timeout
        // Implementation uses 900ms (0.9s) as threshold
        if (silence_ns > 900000000ULL) {
            RC_ASSERT(is_timeout);
        } else {
            RC_ASSERT(!is_timeout);
        }
    });
}

// Feature: tcan1463-q1-simulator, Property 21: Autonomous state transition on wake-up
// **Validates: Requirements 8.6**
TEST(BusBiasControllerPropertyTest, Property21_AutonomousStateTransitionOnWakeUp) {
    rc::check("Autonomous state transition on wake-up property", []() {
        BusBiasController controller;
        bus_bias_controller_init(&controller);
        
        uint64_t current_time = 1000000ULL;
        
        // Start in autonomous inactive state
        bus_bias_controller_update(&controller, CAN_STATE_AUTONOMOUS_INACTIVE, BUS_STATE_RECESSIVE, current_time);
        RC_ASSERT(controller.state == BIAS_STATE_AUTONOMOUS_INACTIVE);
        
        // Simulate remote wake-up by transitioning to autonomous active
        current_time += 1000000ULL;
        bus_bias_controller_update(&controller, CAN_STATE_AUTONOMOUS_ACTIVE, BUS_STATE_DOMINANT, current_time);
        
        // Requirement 8.6: Remote wake-up should transition to autonomous active
        RC_ASSERT(controller.state == BIAS_STATE_AUTONOMOUS_ACTIVE);
        
        // Verify bias voltage is 2.5V
        double canh, canl;
        double vcc = *rc::gen::inRange(3000, 5501) / 1000.0;
        bus_bias_controller_get_bias(&controller, vcc, &canh, &canl);
        RC_ASSERT(canh == 2.5);
        RC_ASSERT(canl == 2.5);
    });
}
