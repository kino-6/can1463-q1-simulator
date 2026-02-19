#include <gtest/gtest.h>
#include "tcan1463q1_c_api.h"

// Test fixture for C API tests
class CAPITest : public ::testing::Test {
protected:
    TCAN1463Q1SimHandle handle;
    
    void SetUp() override {
        handle = nullptr;
    }
    
    void TearDown() override {
        if (handle) {
            tcan_simulator_destroy(handle);
            handle = nullptr;
        }
    }
};

// ========================================================================
// Core Simulator Function Tests
// ========================================================================

TEST_F(CAPITest, CreateAndDestroy) {
    // Test successful creation
    TCAN_ErrorCode result = tcan_simulator_create(&handle);
    EXPECT_EQ(result, TCAN_SUCCESS);
    EXPECT_NE(handle, nullptr);
    
    // Test successful destruction
    result = tcan_simulator_destroy(handle);
    EXPECT_EQ(result, TCAN_SUCCESS);
    handle = nullptr;
}

TEST_F(CAPITest, CreateWithNullPointer) {
    TCAN_ErrorCode result = tcan_simulator_create(nullptr);
    EXPECT_EQ(result, TCAN_ERROR_NULL_POINTER);
}

TEST_F(CAPITest, DestroyWithInvalidHandle) {
    TCAN_ErrorCode result = tcan_simulator_destroy(nullptr);
    EXPECT_EQ(result, TCAN_ERROR_INVALID_HANDLE);
}

TEST_F(CAPITest, Reset) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    // Set some state
    tcan_simulator_set_pin(handle, TCAN_PIN_EN, TCAN_PIN_STATE_HIGH, 3.3);
    
    // Reset should succeed
    TCAN_ErrorCode result = tcan_simulator_reset(handle);
    EXPECT_EQ(result, TCAN_SUCCESS);
    
    // Verify state is reset (mode should be OFF)
    TCAN_OperatingMode mode;
    tcan_simulator_get_mode(handle, &mode);
    EXPECT_EQ(mode, TCAN_MODE_OFF);
}

TEST_F(CAPITest, ResetWithInvalidHandle) {
    TCAN_ErrorCode result = tcan_simulator_reset(nullptr);
    EXPECT_EQ(result, TCAN_ERROR_INVALID_HANDLE);
}

// ========================================================================
// Pin I/O Function Tests
// ========================================================================

TEST_F(CAPITest, SetAndGetPin) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    // Set VSUP to valid voltage
    TCAN_ErrorCode result = tcan_simulator_set_pin(
        handle, TCAN_PIN_VSUP, TCAN_PIN_STATE_ANALOG, 12.0
    );
    EXPECT_EQ(result, TCAN_SUCCESS);
    
    // Get VSUP value
    TCAN_PinState state;
    double voltage;
    result = tcan_simulator_get_pin(handle, TCAN_PIN_VSUP, &state, &voltage);
    EXPECT_EQ(result, TCAN_SUCCESS);
    EXPECT_EQ(state, TCAN_PIN_STATE_ANALOG);
    EXPECT_DOUBLE_EQ(voltage, 12.0);
}

TEST_F(CAPITest, SetPinWithInvalidHandle) {
    TCAN_ErrorCode result = tcan_simulator_set_pin(
        nullptr, TCAN_PIN_EN, TCAN_PIN_STATE_HIGH, 3.3
    );
    EXPECT_EQ(result, TCAN_ERROR_INVALID_HANDLE);
}

TEST_F(CAPITest, SetPinWithInvalidPin) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    TCAN_ErrorCode result = tcan_simulator_set_pin(
        handle, (TCAN_PinType)99, TCAN_PIN_STATE_HIGH, 3.3
    );
    EXPECT_EQ(result, TCAN_ERROR_INVALID_PIN);
}

TEST_F(CAPITest, GetPinWithInvalidHandle) {
    TCAN_PinState state;
    double voltage;
    TCAN_ErrorCode result = tcan_simulator_get_pin(
        nullptr, TCAN_PIN_RXD, &state, &voltage
    );
    EXPECT_EQ(result, TCAN_ERROR_INVALID_HANDLE);
}

TEST_F(CAPITest, GetPinWithInvalidPin) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    TCAN_PinState state;
    double voltage;
    TCAN_ErrorCode result = tcan_simulator_get_pin(
        handle, (TCAN_PinType)99, &state, &voltage
    );
    EXPECT_EQ(result, TCAN_ERROR_INVALID_PIN);
}

TEST_F(CAPITest, SetMultiplePins) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    TCAN_PinValue values[3] = {
        {TCAN_PIN_VSUP, TCAN_PIN_STATE_ANALOG, 12.0},
        {TCAN_PIN_VCC, TCAN_PIN_STATE_ANALOG, 5.0},
        {TCAN_PIN_VIO, TCAN_PIN_STATE_ANALOG, 3.3}
    };
    
    TCAN_ErrorCode result = tcan_simulator_set_pins(handle, values, 3);
    EXPECT_EQ(result, TCAN_SUCCESS);
    
    // Verify values were set
    TCAN_PinState state;
    double voltage;
    tcan_simulator_get_pin(handle, TCAN_PIN_VSUP, &state, &voltage);
    EXPECT_DOUBLE_EQ(voltage, 12.0);
}

TEST_F(CAPITest, GetMultiplePins) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    // Set some pins first
    tcan_simulator_set_pin(handle, TCAN_PIN_VSUP, TCAN_PIN_STATE_ANALOG, 12.0);
    tcan_simulator_set_pin(handle, TCAN_PIN_VCC, TCAN_PIN_STATE_ANALOG, 5.0);
    
    // Get multiple pins
    TCAN_PinValue values[2] = {
        {TCAN_PIN_VSUP, TCAN_PIN_STATE_LOW, 0.0},
        {TCAN_PIN_VCC, TCAN_PIN_STATE_LOW, 0.0}
    };
    
    TCAN_ErrorCode result = tcan_simulator_get_pins(handle, values, 2);
    EXPECT_EQ(result, TCAN_SUCCESS);
    EXPECT_DOUBLE_EQ(values[0].voltage, 12.0);
    EXPECT_DOUBLE_EQ(values[1].voltage, 5.0);
}

TEST_F(CAPITest, GetPinInfo) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    int is_input, is_output;
    double min_voltage, max_voltage;
    
    // Test input pin (TXD)
    TCAN_ErrorCode result = tcan_simulator_get_pin_info(
        handle, TCAN_PIN_TXD, &is_input, &is_output, &min_voltage, &max_voltage
    );
    EXPECT_EQ(result, TCAN_SUCCESS);
    EXPECT_EQ(is_input, 1);
    EXPECT_EQ(is_output, 0);
    
    // Test output pin (RXD)
    result = tcan_simulator_get_pin_info(
        handle, TCAN_PIN_RXD, &is_input, &is_output, &min_voltage, &max_voltage
    );
    EXPECT_EQ(result, TCAN_SUCCESS);
    EXPECT_EQ(is_input, 0);
    EXPECT_EQ(is_output, 1);
}

// ========================================================================
// Simulation Control Function Tests
// ========================================================================

TEST_F(CAPITest, SimulationStep) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    // Step simulation
    TCAN_ErrorCode result = tcan_simulator_step(handle, 1000000); // 1ms
    EXPECT_EQ(result, TCAN_SUCCESS);
}

TEST_F(CAPITest, SimulationStepWithInvalidHandle) {
    TCAN_ErrorCode result = tcan_simulator_step(nullptr, 1000000);
    EXPECT_EQ(result, TCAN_ERROR_INVALID_HANDLE);
}

// Condition callback for run_until test
static int test_condition_callback(TCAN1463Q1SimHandle handle, void* user_data) {
    int* counter = (int*)user_data;
    (*counter)++;
    return (*counter >= 5) ? 1 : 0; // Return true after 5 calls
}

TEST_F(CAPITest, RunUntilCondition) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    int counter = 0;
    TCAN_ErrorCode result = tcan_simulator_run_until(
        handle, test_condition_callback, &counter, 10000000 // 10ms timeout
    );
    EXPECT_EQ(result, TCAN_SUCCESS);
    EXPECT_GE(counter, 5);
}

TEST_F(CAPITest, RunUntilWithInvalidHandle) {
    int counter = 0;
    TCAN_ErrorCode result = tcan_simulator_run_until(
        nullptr, test_condition_callback, &counter, 1000000
    );
    EXPECT_EQ(result, TCAN_ERROR_INVALID_HANDLE);
}

TEST_F(CAPITest, RunUntilWithNullCallback) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    TCAN_ErrorCode result = tcan_simulator_run_until(
        handle, nullptr, nullptr, 1000000
    );
    EXPECT_EQ(result, TCAN_ERROR_NULL_POINTER);
}

// ========================================================================
// State Query Function Tests
// ========================================================================

TEST_F(CAPITest, GetMode) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    TCAN_OperatingMode mode;
    TCAN_ErrorCode result = tcan_simulator_get_mode(handle, &mode);
    EXPECT_EQ(result, TCAN_SUCCESS);
    EXPECT_EQ(mode, TCAN_MODE_OFF); // Initial mode should be OFF
}

TEST_F(CAPITest, GetModeWithInvalidHandle) {
    TCAN_OperatingMode mode;
    TCAN_ErrorCode result = tcan_simulator_get_mode(nullptr, &mode);
    EXPECT_EQ(result, TCAN_ERROR_INVALID_HANDLE);
}

TEST_F(CAPITest, GetModeWithNullPointer) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    TCAN_ErrorCode result = tcan_simulator_get_mode(handle, nullptr);
    EXPECT_EQ(result, TCAN_ERROR_NULL_POINTER);
}

TEST_F(CAPITest, GetFlags) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    int pwron, wakerq, uvsup;
    TCAN_ErrorCode result = tcan_simulator_get_flags(
        handle, &pwron, &wakerq, nullptr, &uvsup, 
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
    );
    EXPECT_EQ(result, TCAN_SUCCESS);
}

TEST_F(CAPITest, GetFlagsWithInvalidHandle) {
    int pwron;
    TCAN_ErrorCode result = tcan_simulator_get_flags(
        nullptr, &pwron, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
    );
    EXPECT_EQ(result, TCAN_ERROR_INVALID_HANDLE);
}

// ========================================================================
// Configuration Function Tests
// ========================================================================

TEST_F(CAPITest, Configure) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    TCAN_ErrorCode result = tcan_simulator_configure(
        handle, 12.0, 5.0, 3.3, 25.0, 60.0, 100e-12
    );
    EXPECT_EQ(result, TCAN_SUCCESS);
}

TEST_F(CAPITest, ConfigureWithInvalidHandle) {
    TCAN_ErrorCode result = tcan_simulator_configure(
        nullptr, 12.0, 5.0, 3.3, 25.0, 60.0, 100e-12
    );
    EXPECT_EQ(result, TCAN_ERROR_INVALID_HANDLE);
}

TEST_F(CAPITest, SetSupplyVoltages) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    TCAN_ErrorCode result = tcan_simulator_set_supply_voltages(
        handle, 12.0, 5.0, 3.3
    );
    EXPECT_EQ(result, TCAN_SUCCESS);
}

TEST_F(CAPITest, SetSupplyVoltagesWithInvalidVoltage) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    // VSUP out of range
    TCAN_ErrorCode result = tcan_simulator_set_supply_voltages(
        handle, 100.0, 5.0, 3.3
    );
    EXPECT_EQ(result, TCAN_ERROR_INVALID_VOLTAGE);
}

TEST_F(CAPITest, SetTemperature) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    TCAN_ErrorCode result = tcan_simulator_set_temperature(handle, 85.0);
    EXPECT_EQ(result, TCAN_SUCCESS);
}

TEST_F(CAPITest, SetTemperatureWithInvalidValue) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    // Temperature out of range
    TCAN_ErrorCode result = tcan_simulator_set_temperature(handle, 300.0);
    EXPECT_EQ(result, TCAN_ERROR_INVALID_PARAMETER);
}

TEST_F(CAPITest, SetBusParameters) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    TCAN_ErrorCode result = tcan_simulator_set_bus_parameters(handle, 60.0, 100e-12);
    EXPECT_EQ(result, TCAN_SUCCESS);
}

TEST_F(CAPITest, SetBusParametersWithInvalidValue) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    // Negative resistance
    TCAN_ErrorCode result = tcan_simulator_set_bus_parameters(handle, -60.0, 100e-12);
    EXPECT_EQ(result, TCAN_ERROR_INVALID_PARAMETER);
}

TEST_F(CAPITest, SetAndGetTimingParameters) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    TCAN_TimingParameters params;
    params.tuv_ms = 200.0;
    params.ttxddto_ms = 2.5;
    params.tbusdom_ms = 2.6;
    params.twk_filter_us = 1.0;
    params.twk_timeout_ms = 1.4;
    params.tsilence_s = 0.9;
    
    TCAN_ErrorCode result = tcan_simulator_set_timing_parameters(handle, &params);
    EXPECT_EQ(result, TCAN_SUCCESS);
    
    // Get timing parameters
    TCAN_TimingParameters retrieved_params;
    result = tcan_simulator_get_timing_parameters(handle, &retrieved_params);
    EXPECT_EQ(result, TCAN_SUCCESS);
    EXPECT_DOUBLE_EQ(retrieved_params.tuv_ms, 200.0);
    EXPECT_DOUBLE_EQ(retrieved_params.ttxddto_ms, 2.5);
}

TEST_F(CAPITest, SetTimingParametersWithInvalidValue) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    TCAN_TimingParameters params;
    params.tuv_ms = 500.0; // Out of range (100-350ms)
    params.ttxddto_ms = 2.5;
    params.tbusdom_ms = 2.6;
    params.twk_filter_us = 1.0;
    params.twk_timeout_ms = 1.4;
    params.tsilence_s = 0.9;
    
    TCAN_ErrorCode result = tcan_simulator_set_timing_parameters(handle, &params);
    EXPECT_EQ(result, TCAN_ERROR_INVALID_PARAMETER);
}

// ========================================================================
// Parameter Validation Function Tests
// ========================================================================

TEST_F(CAPITest, ValidateVsup) {
    EXPECT_EQ(tcan_simulator_validate_vsup(12.0), TCAN_SUCCESS);
    EXPECT_EQ(tcan_simulator_validate_vsup(0.0), TCAN_SUCCESS);
    EXPECT_EQ(tcan_simulator_validate_vsup(40.0), TCAN_SUCCESS);
    EXPECT_EQ(tcan_simulator_validate_vsup(-1.0), TCAN_ERROR_INVALID_VOLTAGE);
    EXPECT_EQ(tcan_simulator_validate_vsup(50.0), TCAN_ERROR_INVALID_VOLTAGE);
}

TEST_F(CAPITest, ValidateVcc) {
    EXPECT_EQ(tcan_simulator_validate_vcc(5.0), TCAN_SUCCESS);
    EXPECT_EQ(tcan_simulator_validate_vcc(0.0), TCAN_SUCCESS);
    EXPECT_EQ(tcan_simulator_validate_vcc(6.0), TCAN_SUCCESS);
    EXPECT_EQ(tcan_simulator_validate_vcc(-1.0), TCAN_ERROR_INVALID_VOLTAGE);
    EXPECT_EQ(tcan_simulator_validate_vcc(10.0), TCAN_ERROR_INVALID_VOLTAGE);
}

TEST_F(CAPITest, ValidateVio) {
    EXPECT_EQ(tcan_simulator_validate_vio(3.3), TCAN_SUCCESS);
    EXPECT_EQ(tcan_simulator_validate_vio(0.0), TCAN_SUCCESS);
    EXPECT_EQ(tcan_simulator_validate_vio(5.5), TCAN_SUCCESS);
    EXPECT_EQ(tcan_simulator_validate_vio(-1.0), TCAN_ERROR_INVALID_VOLTAGE);
    EXPECT_EQ(tcan_simulator_validate_vio(10.0), TCAN_ERROR_INVALID_VOLTAGE);
}

TEST_F(CAPITest, ValidateTemperature) {
    EXPECT_EQ(tcan_simulator_validate_temperature(25.0), TCAN_SUCCESS);
    EXPECT_EQ(tcan_simulator_validate_temperature(-40.0), TCAN_SUCCESS);
    EXPECT_EQ(tcan_simulator_validate_temperature(200.0), TCAN_SUCCESS);
    EXPECT_EQ(tcan_simulator_validate_temperature(-50.0), TCAN_ERROR_INVALID_PARAMETER);
    EXPECT_EQ(tcan_simulator_validate_temperature(250.0), TCAN_ERROR_INVALID_PARAMETER);
}

TEST_F(CAPITest, ValidateTimingParameters) {
    TCAN_TimingParameters valid_params;
    valid_params.tuv_ms = 200.0;
    valid_params.ttxddto_ms = 2.5;
    valid_params.tbusdom_ms = 2.6;
    valid_params.twk_filter_us = 1.0;
    valid_params.twk_timeout_ms = 1.4;
    valid_params.tsilence_s = 0.9;
    
    EXPECT_EQ(tcan_simulator_validate_timing_parameters(&valid_params), TCAN_SUCCESS);
    
    // Invalid tuv_ms
    TCAN_TimingParameters invalid_params = valid_params;
    invalid_params.tuv_ms = 500.0;
    EXPECT_EQ(tcan_simulator_validate_timing_parameters(&invalid_params), TCAN_ERROR_INVALID_PARAMETER);
    
    // Null pointer
    EXPECT_EQ(tcan_simulator_validate_timing_parameters(nullptr), TCAN_ERROR_NULL_POINTER);
}

// ========================================================================
// Snapshot Function Tests
// ========================================================================

TEST_F(CAPITest, SnapshotAndRestore) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    // Set some state
    tcan_simulator_set_pin(handle, TCAN_PIN_VSUP, TCAN_PIN_STATE_ANALOG, 12.0);
    tcan_simulator_set_pin(handle, TCAN_PIN_VCC, TCAN_PIN_STATE_ANALOG, 5.0);
    
    // Create snapshot
    TCAN1463Q1SnapHandle snapshot;
    TCAN_ErrorCode result = tcan_simulator_snapshot(handle, &snapshot);
    EXPECT_EQ(result, TCAN_SUCCESS);
    EXPECT_NE(snapshot, nullptr);
    
    // Modify state
    tcan_simulator_set_pin(handle, TCAN_PIN_VSUP, TCAN_PIN_STATE_ANALOG, 10.0);
    
    // Restore snapshot
    result = tcan_simulator_restore(handle, snapshot);
    EXPECT_EQ(result, TCAN_SUCCESS);
    
    // Verify state was restored
    TCAN_PinState state;
    double voltage;
    tcan_simulator_get_pin(handle, TCAN_PIN_VSUP, &state, &voltage);
    EXPECT_DOUBLE_EQ(voltage, 12.0);
    
    // Free snapshot
    result = tcan_simulator_snapshot_free(snapshot);
    EXPECT_EQ(result, TCAN_SUCCESS);
}

TEST_F(CAPITest, SnapshotWithInvalidHandle) {
    TCAN1463Q1SnapHandle snapshot;
    TCAN_ErrorCode result = tcan_simulator_snapshot(nullptr, &snapshot);
    EXPECT_EQ(result, TCAN_ERROR_INVALID_HANDLE);
}

TEST_F(CAPITest, SnapshotWithNullPointer) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    TCAN_ErrorCode result = tcan_simulator_snapshot(handle, nullptr);
    EXPECT_EQ(result, TCAN_ERROR_NULL_POINTER);
}

TEST_F(CAPITest, RestoreWithInvalidSnapshot) {
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    TCAN_ErrorCode result = tcan_simulator_restore(handle, nullptr);
    EXPECT_EQ(result, TCAN_ERROR_INVALID_SNAPSHOT);
}

// ========================================================================
// Error String Function Tests
// ========================================================================

TEST_F(CAPITest, GetErrorString) {
    const char* msg = tcan_get_error_string(TCAN_SUCCESS);
    EXPECT_STREQ(msg, "Success");
    
    msg = tcan_get_error_string(TCAN_ERROR_INVALID_HANDLE);
    EXPECT_STREQ(msg, "Invalid simulator handle");
    
    msg = tcan_get_error_string(TCAN_ERROR_NULL_POINTER);
    EXPECT_STREQ(msg, "Null pointer argument");
    
    // Unknown error code
    msg = tcan_get_error_string((TCAN_ErrorCode)999);
    EXPECT_STREQ(msg, "Unknown error");
}

// ========================================================================
// Integration Tests
// ========================================================================

TEST_F(CAPITest, BasicSimulationFlow) {
    // Create simulator
    ASSERT_EQ(tcan_simulator_create(&handle), TCAN_SUCCESS);
    
    // Configure power supplies
    ASSERT_EQ(tcan_simulator_set_supply_voltages(handle, 12.0, 5.0, 3.3), TCAN_SUCCESS);
    
    // Set control pins for Normal mode
    ASSERT_EQ(tcan_simulator_set_pin(handle, TCAN_PIN_EN, TCAN_PIN_STATE_HIGH, 3.3), TCAN_SUCCESS);
    ASSERT_EQ(tcan_simulator_set_pin(handle, TCAN_PIN_NSTB, TCAN_PIN_STATE_HIGH, 3.3), TCAN_SUCCESS);
    
    // Step simulation
    ASSERT_EQ(tcan_simulator_step(handle, 1000000), TCAN_SUCCESS); // 1ms
    
    // Check mode transitioned to Normal
    TCAN_OperatingMode mode;
    ASSERT_EQ(tcan_simulator_get_mode(handle, &mode), TCAN_SUCCESS);
    EXPECT_EQ(mode, TCAN_MODE_NORMAL);
    
    // Get flags
    int pwron, wakerq;
    ASSERT_EQ(tcan_simulator_get_flags(
        handle, &pwron, &wakerq, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
    ), TCAN_SUCCESS);
}
