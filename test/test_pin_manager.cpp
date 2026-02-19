#include <gtest/gtest.h>
#include <rapidcheck.h>
#include "pin_manager.h"

class PinManagerTest : public ::testing::Test {
protected:
    PinManager manager;
    
    void SetUp() override {
        pin_manager_init(&manager);
    }
};

// Test pin initialization
TEST_F(PinManagerTest, InitializationSetsDefaultStates) {
    PinState state;
    double voltage;
    
    // Check TXD defaults to HIGH (recessive)
    ASSERT_TRUE(pin_manager_get_pin(&manager, PIN_TXD, &state, &voltage));
    EXPECT_EQ(state, PIN_STATE_HIGH);
    
    // Check RXD defaults to HIGH (recessive)
    ASSERT_TRUE(pin_manager_get_pin(&manager, PIN_RXD, &state, &voltage));
    EXPECT_EQ(state, PIN_STATE_HIGH);
    
    // Check power pins have nominal voltages
    ASSERT_TRUE(pin_manager_get_pin(&manager, PIN_VSUP, &state, &voltage));
    EXPECT_EQ(state, PIN_STATE_ANALOG);
    EXPECT_EQ(voltage, 12.0);
    
    ASSERT_TRUE(pin_manager_get_pin(&manager, PIN_VCC, &state, &voltage));
    EXPECT_EQ(voltage, 5.0);
    
    ASSERT_TRUE(pin_manager_get_pin(&manager, PIN_VIO, &state, &voltage));
    EXPECT_EQ(voltage, 3.3);
}

// Test setting input pins
TEST_F(PinManagerTest, CanSetInputPins) {
    // Set TXD to LOW
    ASSERT_TRUE(pin_manager_set_pin(&manager, PIN_TXD, PIN_STATE_LOW, 0.0));
    
    PinState state;
    double voltage;
    ASSERT_TRUE(pin_manager_get_pin(&manager, PIN_TXD, &state, &voltage));
    EXPECT_EQ(state, PIN_STATE_LOW);
}

// Test cannot set output-only pins
TEST_F(PinManagerTest, CannotSetOutputOnlyPins) {
    // RXD is output-only, should fail
    EXPECT_FALSE(pin_manager_set_pin(&manager, PIN_RXD, PIN_STATE_LOW, 0.0));
    
    // nFAULT is output-only, should fail
    EXPECT_FALSE(pin_manager_set_pin(&manager, PIN_NFAULT, PIN_STATE_LOW, 0.0));
}

// Test voltage range validation - valid voltages
TEST_F(PinManagerTest, AcceptsValidVoltages) {
    // VSUP valid range: 4.5V to 42V
    EXPECT_TRUE(pin_manager_validate_pin(&manager, PIN_VSUP, 12.0));
    EXPECT_TRUE(pin_manager_validate_pin(&manager, PIN_VSUP, 4.5));
    EXPECT_TRUE(pin_manager_validate_pin(&manager, PIN_VSUP, 42.0));
    
    // VCC valid range: 4.5V to 5.5V
    EXPECT_TRUE(pin_manager_validate_pin(&manager, PIN_VCC, 5.0));
    EXPECT_TRUE(pin_manager_validate_pin(&manager, PIN_VCC, 4.5));
    EXPECT_TRUE(pin_manager_validate_pin(&manager, PIN_VCC, 5.5));
    
    // VIO valid range: 1.65V to 5.5V
    EXPECT_TRUE(pin_manager_validate_pin(&manager, PIN_VIO, 3.3));
    EXPECT_TRUE(pin_manager_validate_pin(&manager, PIN_VIO, 1.65));
    EXPECT_TRUE(pin_manager_validate_pin(&manager, PIN_VIO, 5.5));
}

// Test voltage range validation - invalid voltages (Requirement 1.5)
TEST_F(PinManagerTest, RejectsInvalidVoltages) {
    // VSUP below minimum
    EXPECT_FALSE(pin_manager_validate_pin(&manager, PIN_VSUP, 4.0));
    
    // VSUP above maximum
    EXPECT_FALSE(pin_manager_validate_pin(&manager, PIN_VSUP, 43.0));
    
    // VCC below minimum
    EXPECT_FALSE(pin_manager_validate_pin(&manager, PIN_VCC, 4.0));
    
    // VCC above maximum
    EXPECT_FALSE(pin_manager_validate_pin(&manager, PIN_VCC, 6.0));
    
    // VIO below minimum
    EXPECT_FALSE(pin_manager_validate_pin(&manager, PIN_VIO, 1.0));
    
    // VIO above maximum
    EXPECT_FALSE(pin_manager_validate_pin(&manager, PIN_VIO, 6.0));
}

// Test setting pin with invalid voltage fails (Requirement 1.5)
TEST_F(PinManagerTest, SetPinRejectsInvalidVoltage) {
    // Try to set VSUP to invalid voltage
    EXPECT_FALSE(pin_manager_set_pin(&manager, PIN_VSUP, PIN_STATE_ANALOG, 50.0));
    
    // Verify voltage wasn't changed
    PinState state;
    double voltage;
    ASSERT_TRUE(pin_manager_get_pin(&manager, PIN_VSUP, &state, &voltage));
    EXPECT_EQ(voltage, 12.0);  // Should still be default
}

// Test pin value round-trip (Requirement 1.6)
TEST_F(PinManagerTest, PinValueRoundTrip) {
    // Set VSUP to a specific voltage
    ASSERT_TRUE(pin_manager_set_pin(&manager, PIN_VSUP, PIN_STATE_ANALOG, 13.5));
    
    // Read it back
    PinState state;
    double voltage;
    ASSERT_TRUE(pin_manager_get_pin(&manager, PIN_VSUP, &state, &voltage));
    EXPECT_EQ(state, PIN_STATE_ANALOG);
    EXPECT_DOUBLE_EQ(voltage, 13.5);
    
    // Test with digital pin
    ASSERT_TRUE(pin_manager_set_pin(&manager, PIN_EN, PIN_STATE_HIGH, 0.0));
    ASSERT_TRUE(pin_manager_get_pin(&manager, PIN_EN, &state, &voltage));
    EXPECT_EQ(state, PIN_STATE_HIGH);
}

// Test bidirectional pins can be set
TEST_F(PinManagerTest, CanSetBidirectionalPins) {
    // CANH and CANL are bidirectional
    ASSERT_TRUE(pin_manager_set_pin(&manager, PIN_CANH, PIN_STATE_ANALOG, 3.5));
    ASSERT_TRUE(pin_manager_set_pin(&manager, PIN_CANL, PIN_STATE_ANALOG, 1.5));
    
    PinState state;
    double voltage;
    ASSERT_TRUE(pin_manager_get_pin(&manager, PIN_CANH, &state, &voltage));
    EXPECT_EQ(state, PIN_STATE_ANALOG);
    EXPECT_DOUBLE_EQ(voltage, 3.5);
    
    ASSERT_TRUE(pin_manager_get_pin(&manager, PIN_CANL, &state, &voltage));
    EXPECT_EQ(state, PIN_STATE_ANALOG);
    EXPECT_DOUBLE_EQ(voltage, 1.5);
}

// Test get pin info
TEST_F(PinManagerTest, GetPinInfoReturnsCorrectMetadata) {
    bool is_input, is_output;
    double min_voltage, max_voltage;
    
    // Check TXD (input only)
    ASSERT_TRUE(pin_manager_get_pin_info(&manager, PIN_TXD, 
                                         &is_input, &is_output,
                                         &min_voltage, &max_voltage));
    EXPECT_TRUE(is_input);
    EXPECT_FALSE(is_output);
    EXPECT_DOUBLE_EQ(min_voltage, 0.0);
    EXPECT_DOUBLE_EQ(max_voltage, 5.5);
    
    // Check RXD (output only)
    ASSERT_TRUE(pin_manager_get_pin_info(&manager, PIN_RXD,
                                         &is_input, &is_output,
                                         &min_voltage, &max_voltage));
    EXPECT_FALSE(is_input);
    EXPECT_TRUE(is_output);
    
    // Check CANH (bidirectional)
    ASSERT_TRUE(pin_manager_get_pin_info(&manager, PIN_CANH,
                                         &is_input, &is_output,
                                         &min_voltage, &max_voltage));
    EXPECT_TRUE(is_input);
    EXPECT_TRUE(is_output);
    EXPECT_DOUBLE_EQ(min_voltage, -27.0);
    EXPECT_DOUBLE_EQ(max_voltage, 42.0);
}

// Test invalid pin type
TEST_F(PinManagerTest, RejectsInvalidPinType) {
    PinState state;
    double voltage;
    
    // Invalid pin type (out of range)
    EXPECT_FALSE(pin_manager_get_pin(&manager, (PinType)99, &state, &voltage));
    EXPECT_FALSE(pin_manager_set_pin(&manager, (PinType)99, PIN_STATE_LOW, 0.0));
    EXPECT_FALSE(pin_manager_validate_pin(&manager, (PinType)99, 5.0));
}

// Test null pointer handling
TEST_F(PinManagerTest, HandlesNullPointers) {
    EXPECT_FALSE(pin_manager_set_pin(nullptr, PIN_TXD, PIN_STATE_LOW, 0.0));
    EXPECT_FALSE(pin_manager_get_pin(nullptr, PIN_TXD, nullptr, nullptr));
    EXPECT_FALSE(pin_manager_validate_pin(nullptr, PIN_TXD, 5.0));
    EXPECT_FALSE(pin_manager_get_pin_info(nullptr, PIN_TXD, nullptr, nullptr, nullptr, nullptr));
}

// Test all digital input pins (Requirements 1.1)
TEST_F(PinManagerTest, AllDigitalInputPinsWork) {
    PinType digital_inputs[] = {PIN_TXD, PIN_EN, PIN_NSTB, PIN_WAKE, PIN_INH_MASK};
    
    for (PinType pin : digital_inputs) {
        ASSERT_TRUE(pin_manager_set_pin(&manager, pin, PIN_STATE_HIGH, 0.0));
        
        PinState state;
        ASSERT_TRUE(pin_manager_get_pin(&manager, pin, &state, nullptr));
        EXPECT_EQ(state, PIN_STATE_HIGH);
    }
}

// Test all power supply pins (Requirements 1.4)
TEST_F(PinManagerTest, AllPowerSupplyPinsWork) {
    // Set VSUP
    ASSERT_TRUE(pin_manager_set_pin(&manager, PIN_VSUP, PIN_STATE_ANALOG, 14.0));
    
    // Set VCC
    ASSERT_TRUE(pin_manager_set_pin(&manager, PIN_VCC, PIN_STATE_ANALOG, 5.0));
    
    // Set VIO
    ASSERT_TRUE(pin_manager_set_pin(&manager, PIN_VIO, PIN_STATE_ANALOG, 3.3));
    
    // Verify all were set correctly
    PinState state;
    double voltage;
    
    ASSERT_TRUE(pin_manager_get_pin(&manager, PIN_VSUP, &state, &voltage));
    EXPECT_DOUBLE_EQ(voltage, 14.0);
    
    ASSERT_TRUE(pin_manager_get_pin(&manager, PIN_VCC, &state, &voltage));
    EXPECT_DOUBLE_EQ(voltage, 5.0);
    
    ASSERT_TRUE(pin_manager_get_pin(&manager, PIN_VIO, &state, &voltage));
    EXPECT_DOUBLE_EQ(voltage, 3.3);
}

// ============================================================================
// Property-Based Tests
// ============================================================================

// Feature: tcan1463-q1-simulator, Property 1: Pin voltage validation
// **Validates: Requirements 1.5**
//
// Property: For any pin and voltage value, when setting a pin value,
// if the voltage is outside the pin's valid range, the operation should
// be rejected and return false.
TEST(PinManagerPropertyTest, PinVoltageValidation) {
    rc::check("Pin voltage validation property", []() {
        PinManager manager;
        pin_manager_init(&manager);
        
        // Generate a random pin type (0-13)
        const auto pin_type = *rc::gen::inRange(0, 14);
        
        // Get the pin's valid voltage range
        bool is_input, is_output;
        double min_voltage, max_voltage;
        RC_ASSERT(pin_manager_get_pin_info(&manager, (PinType)pin_type,
                                           &is_input, &is_output,
                                           &min_voltage, &max_voltage));
        
        // Generate a voltage value using arbitrary<double> and scale it
        const auto voltage_raw = *rc::gen::arbitrary<double>();
        // Scale to a reasonable range for testing (-50V to 100V)
        const double voltage = voltage_raw * 75.0;  // Will give us a wide range
        
        // Determine if voltage is within valid range
        const bool is_valid = (voltage >= min_voltage && voltage <= max_voltage);
        
        // For input pins, test set_pin operation
        if (is_input) {
            const bool result = pin_manager_set_pin(&manager, (PinType)pin_type,
                                                    PIN_STATE_ANALOG, voltage);
            
            // Property: If voltage is outside valid range, operation should fail
            if (!is_valid) {
                RC_ASSERT(!result);  // Should return false for invalid voltage
                
                // Verify the pin value wasn't changed
                PinState state;
                double current_voltage;
                RC_ASSERT(pin_manager_get_pin(&manager, (PinType)pin_type,
                                             &state, &current_voltage));
                // The voltage should still be the default value, not the invalid one
                RC_ASSERT(current_voltage != voltage);
            } else {
                // For valid voltages, operation should succeed
                RC_ASSERT(result);
                
                // Verify the pin value was set correctly
                PinState state;
                double current_voltage;
                RC_ASSERT(pin_manager_get_pin(&manager, (PinType)pin_type,
                                             &state, &current_voltage));
                RC_ASSERT(current_voltage == voltage);
            }
        }
        
        // Test validate_pin operation for all pins
        const bool validate_result = pin_manager_validate_pin(&manager,
                                                              (PinType)pin_type,
                                                              voltage);
        
        // Property: validate_pin should return true iff voltage is in valid range
        RC_ASSERT(validate_result == is_valid);
    });
}

// Feature: tcan1463-q1-simulator, Property 2: Pin value round-trip consistency
// **Validates: Requirements 1.6, 11.2**
//
// Property: For any input pin and valid voltage value, after setting the pin
// value via the API, reading the pin value should return the same state and voltage.
TEST(PinManagerPropertyTest, PinValueRoundTripConsistency) {
    rc::check("Pin value round-trip consistency property", []() {
        PinManager manager;
        pin_manager_init(&manager);
        
        // Generate a random pin type (0-13)
        const auto pin_type = *rc::gen::inRange(0, 14);
        
        // Get the pin's properties
        bool is_input, is_output;
        double min_voltage, max_voltage;
        RC_ASSERT(pin_manager_get_pin_info(&manager, (PinType)pin_type,
                                           &is_input, &is_output,
                                           &min_voltage, &max_voltage));
        
        // Only test input pins (pins that can be set externally)
        if (!is_input) {
            RC_SUCCEED();  // Skip output-only pins
            return;
        }
        
        // Generate a valid voltage within the pin's range
        // Use arbitrary<double> and scale it to the valid range
        const auto voltage_factor = *rc::gen::inRange(0, 1001);  // 0 to 1000
        const double voltage = min_voltage + (voltage_factor / 1000.0) * (max_voltage - min_voltage);
        
        // Generate a random pin state
        const auto state_value = *rc::gen::inRange(0, 4);
        const PinState state = (PinState)state_value;
        
        // Set the pin value
        const bool set_result = pin_manager_set_pin(&manager, (PinType)pin_type,
                                                     state, voltage);
        
        // For valid voltages, set should succeed
        RC_ASSERT(set_result);
        
        // Read the pin value back
        PinState read_state;
        double read_voltage;
        const bool get_result = pin_manager_get_pin(&manager, (PinType)pin_type,
                                                     &read_state, &read_voltage);
        
        // Get should always succeed
        RC_ASSERT(get_result);
        
        // Property: The read values should match what was set
        RC_ASSERT(read_state == state);
        RC_ASSERT(read_voltage == voltage);
    });
}
