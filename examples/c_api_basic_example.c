/**
 * @file c_api_basic_example.c
 * @brief Basic usage example of TCAN1463-Q1 Simulator C API
 * 
 * This example demonstrates:
 * - Creating and destroying a simulator instance
 * - Configuring power supplies
 * - Setting control pins
 * - Running simulation steps
 * - Reading output pins and status
 */

#include "tcan1463q1_c_api.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    TCAN1463Q1SimHandle sim = NULL;
    TCAN_ErrorCode result;
    
    printf("=== TCAN1463-Q1 Simulator C API Basic Example ===\n\n");
    
    // Step 1: Create simulator instance
    printf("1. Creating simulator instance...\n");
    result = tcan_simulator_create(&sim);
    if (result != TCAN_SUCCESS) {
        fprintf(stderr, "Error creating simulator: %s\n", 
                tcan_get_error_string(result));
        return EXIT_FAILURE;
    }
    printf("   Simulator created successfully\n\n");
    
    // Step 2: Configure power supplies
    printf("2. Configuring power supplies...\n");
    result = tcan_simulator_set_supply_voltages(sim, 12.0, 5.0, 3.3);
    if (result != TCAN_SUCCESS) {
        fprintf(stderr, "Error setting supply voltages: %s\n", 
                tcan_get_error_string(result));
        tcan_simulator_destroy(sim);
        return EXIT_FAILURE;
    }
    printf("   VSUP = 12.0V, VCC = 5.0V, VIO = 3.3V\n\n");
    
    // Step 3: Set control pins for Normal mode
    printf("3. Setting control pins for Normal mode...\n");
    result = tcan_simulator_set_pin(sim, TCAN_PIN_EN, TCAN_PIN_STATE_HIGH, 3.3);
    if (result != TCAN_SUCCESS) {
        fprintf(stderr, "Error setting EN pin: %s\n", 
                tcan_get_error_string(result));
        tcan_simulator_destroy(sim);
        return EXIT_FAILURE;
    }
    
    result = tcan_simulator_set_pin(sim, TCAN_PIN_NSTB, TCAN_PIN_STATE_HIGH, 3.3);
    if (result != TCAN_SUCCESS) {
        fprintf(stderr, "Error setting nSTB pin: %s\n", 
                tcan_get_error_string(result));
        tcan_simulator_destroy(sim);
        return EXIT_FAILURE;
    }
    printf("   EN = HIGH, nSTB = HIGH\n\n");
    
    // Step 4: Run simulation for 1ms to allow mode transition
    printf("4. Running simulation for 1ms...\n");
    result = tcan_simulator_step(sim, 1000000); // 1ms in nanoseconds
    if (result != TCAN_SUCCESS) {
        fprintf(stderr, "Error stepping simulation: %s\n", 
                tcan_get_error_string(result));
        tcan_simulator_destroy(sim);
        return EXIT_FAILURE;
    }
    printf("   Simulation advanced by 1ms\n\n");
    
    // Step 5: Check operating mode
    printf("5. Checking operating mode...\n");
    TCAN_OperatingMode mode;
    result = tcan_simulator_get_mode(sim, &mode);
    if (result != TCAN_SUCCESS) {
        fprintf(stderr, "Error getting mode: %s\n", 
                tcan_get_error_string(result));
        tcan_simulator_destroy(sim);
        return EXIT_FAILURE;
    }
    
    const char* mode_names[] = {
        "NORMAL", "SILENT", "STANDBY", "GO_TO_SLEEP", "SLEEP", "OFF"
    };
    printf("   Current mode: %s\n\n", mode_names[mode]);
    
    // Step 6: Send a CAN message (set TXD low for dominant bit)
    printf("6. Sending CAN dominant bit (TXD = LOW)...\n");
    result = tcan_simulator_set_pin(sim, TCAN_PIN_TXD, TCAN_PIN_STATE_LOW, 0.0);
    if (result != TCAN_SUCCESS) {
        fprintf(stderr, "Error setting TXD pin: %s\n", 
                tcan_get_error_string(result));
        tcan_simulator_destroy(sim);
        return EXIT_FAILURE;
    }
    
    // Step simulation to propagate signal
    result = tcan_simulator_step(sim, 1000); // 1us
    if (result != TCAN_SUCCESS) {
        fprintf(stderr, "Error stepping simulation: %s\n", 
                tcan_get_error_string(result));
        tcan_simulator_destroy(sim);
        return EXIT_FAILURE;
    }
    printf("   TXD set to LOW\n\n");
    
    // Step 7: Read RXD output
    printf("7. Reading RXD output...\n");
    TCAN_PinState rxd_state;
    double rxd_voltage;
    result = tcan_simulator_get_pin(sim, TCAN_PIN_RXD, &rxd_state, &rxd_voltage);
    if (result != TCAN_SUCCESS) {
        fprintf(stderr, "Error getting RXD pin: %s\n", 
                tcan_get_error_string(result));
        tcan_simulator_destroy(sim);
        return EXIT_FAILURE;
    }
    
    const char* state_names[] = {"LOW", "HIGH", "HIGH_IMPEDANCE", "ANALOG"};
    printf("   RXD state: %s, voltage: %.2fV\n\n", 
           state_names[rxd_state], rxd_voltage);
    
    // Step 8: Read CAN bus voltages
    printf("8. Reading CAN bus voltages...\n");
    TCAN_PinState canh_state, canl_state;
    double canh_voltage, canl_voltage;
    
    result = tcan_simulator_get_pin(sim, TCAN_PIN_CANH, &canh_state, &canh_voltage);
    if (result != TCAN_SUCCESS) {
        fprintf(stderr, "Error getting CANH pin: %s\n", 
                tcan_get_error_string(result));
        tcan_simulator_destroy(sim);
        return EXIT_FAILURE;
    }
    
    result = tcan_simulator_get_pin(sim, TCAN_PIN_CANL, &canl_state, &canl_voltage);
    if (result != TCAN_SUCCESS) {
        fprintf(stderr, "Error getting CANL pin: %s\n", 
                tcan_get_error_string(result));
        tcan_simulator_destroy(sim);
        return EXIT_FAILURE;
    }
    
    printf("   CANH: %.2fV, CANL: %.2fV\n", canh_voltage, canl_voltage);
    printf("   Differential voltage: %.2fV\n\n", canh_voltage - canl_voltage);
    
    // Step 9: Get status flags
    printf("9. Reading status flags...\n");
    int pwron, wakerq, uvsup, uvcc, uvio;
    result = tcan_simulator_get_flags(
        sim, &pwron, &wakerq, NULL, &uvsup, &uvcc, &uvio,
        NULL, NULL, NULL, NULL, NULL, NULL
    );
    if (result != TCAN_SUCCESS) {
        fprintf(stderr, "Error getting flags: %s\n", 
                tcan_get_error_string(result));
        tcan_simulator_destroy(sim);
        return EXIT_FAILURE;
    }
    
    printf("   PWRON: %d, WAKERQ: %d\n", pwron, wakerq);
    printf("   UVSUP: %d, UVCC: %d, UVIO: %d\n\n", uvsup, uvcc, uvio);
    
    // Step 10: Clean up
    printf("10. Destroying simulator...\n");
    result = tcan_simulator_destroy(sim);
    if (result != TCAN_SUCCESS) {
        fprintf(stderr, "Error destroying simulator: %s\n", 
                tcan_get_error_string(result));
        return EXIT_FAILURE;
    }
    printf("    Simulator destroyed successfully\n\n");
    
    printf("=== Example completed successfully ===\n");
    return EXIT_SUCCESS;
}
