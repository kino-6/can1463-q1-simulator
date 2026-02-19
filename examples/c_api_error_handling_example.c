/**
 * @file c_api_error_handling_example.c
 * @brief Error handling example for TCAN1463-Q1 Simulator C API
 * 
 * This example demonstrates:
 * - Proper error checking for all API calls
 * - Handling invalid parameters
 * - Detecting and responding to fault conditions
 * - Using snapshots for state recovery
 */

#include "tcan1463q1_c_api.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Helper function to print error and clean up
 */
static void handle_error(const char* operation, TCAN_ErrorCode error, 
                        TCAN1463Q1SimHandle sim) {
    fprintf(stderr, "ERROR during %s: %s\n", operation, 
            tcan_get_error_string(error));
    if (sim) {
        tcan_simulator_destroy(sim);
    }
}

/**
 * @brief Example 1: Handling invalid parameters
 */
static int example_invalid_parameters(void) {
    printf("\n=== Example 1: Handling Invalid Parameters ===\n\n");
    
    TCAN1463Q1SimHandle sim = NULL;
    TCAN_ErrorCode result;
    
    // Create simulator
    result = tcan_simulator_create(&sim);
    if (result != TCAN_SUCCESS) {
        fprintf(stderr, "Failed to create simulator: %s\n", 
                tcan_get_error_string(result));
        return EXIT_FAILURE;
    }
    
    // Try to set invalid voltage (out of range)
    printf("1. Attempting to set invalid VSUP voltage (100V)...\n");
    result = tcan_simulator_set_supply_voltages(sim, 100.0, 5.0, 3.3);
    if (result != TCAN_SUCCESS) {
        printf("   Expected error caught: %s\n", tcan_get_error_string(result));
    } else {
        printf("   ERROR: Invalid voltage was accepted!\n");
    }
    
    // Try to set invalid temperature
    printf("\n2. Attempting to set invalid temperature (300°C)...\n");
    result = tcan_simulator_set_temperature(sim, 300.0);
    if (result != TCAN_SUCCESS) {
        printf("   Expected error caught: %s\n", tcan_get_error_string(result));
    } else {
        printf("   ERROR: Invalid temperature was accepted!\n");
    }
    
    // Try to set invalid timing parameters
    printf("\n3. Attempting to set invalid timing parameters...\n");
    TCAN_TimingParameters invalid_params;
    invalid_params.tuv_ms = 500.0; // Out of range (100-350ms)
    invalid_params.ttxddto_ms = 2.5;
    invalid_params.tbusdom_ms = 2.6;
    invalid_params.twk_filter_us = 1.0;
    invalid_params.twk_timeout_ms = 1.4;
    invalid_params.tsilence_s = 0.9;
    
    result = tcan_simulator_set_timing_parameters(sim, &invalid_params);
    if (result != TCAN_SUCCESS) {
        printf("   Expected error caught: %s\n", tcan_get_error_string(result));
    } else {
        printf("   ERROR: Invalid timing parameters were accepted!\n");
    }
    
    // Validate parameters before setting
    printf("\n4. Validating parameters before setting...\n");
    double test_vsup = 12.0;
    result = tcan_simulator_validate_vsup(test_vsup);
    if (result == TCAN_SUCCESS) {
        printf("   VSUP %.1fV is valid\n", test_vsup);
        tcan_simulator_set_supply_voltages(sim, test_vsup, 5.0, 3.3);
    }
    
    tcan_simulator_destroy(sim);
    printf("\n=== Example 1 completed ===\n");
    return EXIT_SUCCESS;
}

/**
 * @brief Example 2: Detecting and handling fault conditions
 */
static int example_fault_detection(void) {
    printf("\n=== Example 2: Detecting Fault Conditions ===\n\n");
    
    TCAN1463Q1SimHandle sim = NULL;
    TCAN_ErrorCode result;
    
    // Create and configure simulator
    result = tcan_simulator_create(&sim);
    if (result != TCAN_SUCCESS) {
        fprintf(stderr, "Failed to create simulator: %s\n", 
                tcan_get_error_string(result));
        return EXIT_FAILURE;
    }
    
    // Configure for normal operation
    tcan_simulator_set_supply_voltages(sim, 12.0, 5.0, 3.3);
    tcan_simulator_set_pin(sim, TCAN_PIN_EN, TCAN_PIN_STATE_HIGH, 3.3);
    tcan_simulator_set_pin(sim, TCAN_PIN_NSTB, TCAN_PIN_STATE_HIGH, 3.3);
    tcan_simulator_step(sim, 1000000); // 1ms
    
    printf("1. Simulating TXD dominant timeout fault...\n");
    
    // Set TXD low (dominant) and hold it
    tcan_simulator_set_pin(sim, TCAN_PIN_TXD, TCAN_PIN_STATE_LOW, 0.0);
    
    // Run simulation for longer than TXDDTO timeout (>3.8ms)
    tcan_simulator_step(sim, 5000000); // 5ms
    
    // Check for fault flags
    int txddto_flag, nfault_state;
    TCAN_PinState nfault_pin_state;
    double nfault_voltage;
    
    result = tcan_simulator_get_flags(
        sim, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, &txddto_flag, NULL, NULL, NULL
    );
    
    if (result == TCAN_SUCCESS && txddto_flag) {
        printf("   TXDDTO fault detected!\n");
        
        // Check nFAULT pin
        tcan_simulator_get_pin(sim, TCAN_PIN_NFAULT, &nfault_pin_state, &nfault_voltage);
        printf("   nFAULT pin state: %s\n", 
               nfault_pin_state == TCAN_PIN_STATE_LOW ? "LOW (fault indicated)" : "HIGH");
    }
    
    printf("\n2. Simulating thermal shutdown...\n");
    
    // Reset simulator
    tcan_simulator_reset(sim);
    tcan_simulator_set_supply_voltages(sim, 12.0, 5.0, 3.3);
    tcan_simulator_set_pin(sim, TCAN_PIN_EN, TCAN_PIN_STATE_HIGH, 3.3);
    tcan_simulator_set_pin(sim, TCAN_PIN_NSTB, TCAN_PIN_STATE_HIGH, 3.3);
    
    // Set high temperature
    tcan_simulator_set_temperature(sim, 170.0); // Above TSD threshold (165°C)
    tcan_simulator_step(sim, 1000000); // 1ms
    
    // Check for TSD flag
    int tsd_flag;
    result = tcan_simulator_get_flags(
        sim, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, &tsd_flag
    );
    
    if (result == TCAN_SUCCESS && tsd_flag) {
        printf("   Thermal shutdown detected!\n");
        printf("   CAN driver should be disabled\n");
    }
    
    printf("\n3. Simulating undervoltage condition...\n");
    
    // Reset and set low supply voltage
    tcan_simulator_reset(sim);
    tcan_simulator_set_supply_voltages(sim, 3.0, 3.0, 1.0); // Below thresholds
    tcan_simulator_step(sim, 200000000); // 200ms (longer than tUV)
    
    // Check UV flags
    int uvsup, uvcc, uvio;
    result = tcan_simulator_get_flags(
        sim, NULL, NULL, NULL, &uvsup, &uvcc, &uvio,
        NULL, NULL, NULL, NULL, NULL, NULL
    );
    
    if (result == TCAN_SUCCESS) {
        printf("   Undervoltage flags:\n");
        printf("   UVSUP: %s\n", uvsup ? "SET" : "CLEAR");
        printf("   UVCC: %s\n", uvcc ? "SET" : "CLEAR");
        printf("   UVIO: %s\n", uvio ? "SET" : "CLEAR");
    }
    
    tcan_simulator_destroy(sim);
    printf("\n=== Example 2 completed ===\n");
    return EXIT_SUCCESS;
}

/**
 * @brief Example 3: Using snapshots for state recovery
 */
static int example_snapshot_recovery(void) {
    printf("\n=== Example 3: Using Snapshots for State Recovery ===\n\n");
    
    TCAN1463Q1SimHandle sim = NULL;
    TCAN1463Q1SnapHandle snapshot = NULL;
    TCAN_ErrorCode result;
    
    // Create and configure simulator
    result = tcan_simulator_create(&sim);
    if (result != TCAN_SUCCESS) {
        fprintf(stderr, "Failed to create simulator: %s\n", 
                tcan_get_error_string(result));
        return EXIT_FAILURE;
    }
    
    printf("1. Setting up known good state...\n");
    tcan_simulator_set_supply_voltages(sim, 12.0, 5.0, 3.3);
    tcan_simulator_set_pin(sim, TCAN_PIN_EN, TCAN_PIN_STATE_HIGH, 3.3);
    tcan_simulator_set_pin(sim, TCAN_PIN_NSTB, TCAN_PIN_STATE_HIGH, 3.3);
    tcan_simulator_step(sim, 1000000); // 1ms
    
    TCAN_OperatingMode mode;
    tcan_simulator_get_mode(sim, &mode);
    printf("   Mode: %s\n", mode == TCAN_MODE_NORMAL ? "NORMAL" : "OTHER");
    
    printf("\n2. Creating snapshot of good state...\n");
    result = tcan_simulator_snapshot(sim, &snapshot);
    if (result != TCAN_SUCCESS) {
        handle_error("snapshot creation", result, sim);
        return EXIT_FAILURE;
    }
    printf("   Snapshot created successfully\n");
    
    printf("\n3. Modifying state (simulating error condition)...\n");
    tcan_simulator_set_supply_voltages(sim, 3.0, 3.0, 1.0); // Undervoltage
    tcan_simulator_step(sim, 200000000); // 200ms
    
    tcan_simulator_get_mode(sim, &mode);
    printf("   Mode after error: %s\n", mode == TCAN_MODE_OFF ? "OFF" : "OTHER");
    
    printf("\n4. Restoring from snapshot...\n");
    result = tcan_simulator_restore(sim, snapshot);
    if (result != TCAN_SUCCESS) {
        handle_error("snapshot restore", result, sim);
        tcan_simulator_snapshot_free(snapshot);
        return EXIT_FAILURE;
    }
    printf("   State restored successfully\n");
    
    tcan_simulator_get_mode(sim, &mode);
    printf("   Mode after restore: %s\n", mode == TCAN_MODE_NORMAL ? "NORMAL" : "OTHER");
    
    // Verify voltages were restored
    TCAN_PinState state;
    double voltage;
    tcan_simulator_get_pin(sim, TCAN_PIN_VSUP, &state, &voltage);
    printf("   VSUP after restore: %.1fV\n", voltage);
    
    printf("\n5. Cleaning up...\n");
    result = tcan_simulator_snapshot_free(snapshot);
    if (result != TCAN_SUCCESS) {
        handle_error("snapshot free", result, sim);
        return EXIT_FAILURE;
    }
    printf("   Snapshot freed\n");
    
    tcan_simulator_destroy(sim);
    printf("   Simulator destroyed\n");
    
    printf("\n=== Example 3 completed ===\n");
    return EXIT_SUCCESS;
}

/**
 * @brief Main function - runs all examples
 */
int main(void) {
    printf("=== TCAN1463-Q1 Simulator C API Error Handling Examples ===\n");
    
    int result = EXIT_SUCCESS;
    
    // Run all examples
    if (example_invalid_parameters() != EXIT_SUCCESS) {
        result = EXIT_FAILURE;
    }
    
    if (example_fault_detection() != EXIT_SUCCESS) {
        result = EXIT_FAILURE;
    }
    
    if (example_snapshot_recovery() != EXIT_SUCCESS) {
        result = EXIT_FAILURE;
    }
    
    printf("\n=== All examples completed ===\n");
    return result;
}
