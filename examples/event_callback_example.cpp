#include "tcan1463q1_simulator.h"
#include <stdio.h>

// Event counters
static int mode_change_count = 0;
static int fault_count = 0;

// Mode change callback
void on_mode_change(const SimulatorEvent* event, void* user_data) {
    mode_change_count++;
    printf("[EVENT] Mode changed: %d -> %d (timestamp: %llu ns)\n",
           event->data.mode_change.old_mode,
           event->data.mode_change.new_mode,
           event->timestamp);
}

// Fault detected callback
void on_fault_detected(const SimulatorEvent* event, void* user_data) {
    fault_count++;
    printf("[EVENT] Fault detected: %s (set: %d, timestamp: %llu ns)\n",
           event->data.fault.fault_name,
           event->data.fault.is_set,
           event->timestamp);
}

int main() {
    printf("=== TCAN1463-Q1 Event Callback Example ===\n\n");
    
    // Create simulator
    TCAN1463Q1Simulator* sim = tcan1463q1_simulator_create();
    if (!sim) {
        printf("Failed to create simulator\n");
        return 1;
    }
    
    printf("1. Registering event callbacks...\n");
    bool reg1 = tcan1463q1_simulator_register_callback(
        sim, EVENT_MODE_CHANGE, on_mode_change, NULL);
    bool reg2 = tcan1463q1_simulator_register_callback(
        sim, EVENT_FAULT_DETECTED, on_fault_detected, NULL);
    
    if (reg1 && reg2) {
        printf("   Callbacks registered successfully\n\n");
    } else {
        printf("   Failed to register callbacks\n");
        tcan1463q1_simulator_destroy(sim);
        return 1;
    }
    
    printf("2. Configuring power supplies...\n");
    tcan1463q1_simulator_configure(sim, 12.0, 5.0, 3.3, 25.0, 60.0, 100e-12);
    printf("   VSUP = 12.0V, VCC = 5.0V, VIO = 3.3V\n\n");
    
    printf("3. Transitioning to Normal mode...\n");
    tcan1463q1_simulator_set_pin(sim, PIN_EN, PIN_STATE_HIGH, 3.3);
    tcan1463q1_simulator_set_pin(sim, PIN_NSTB, PIN_STATE_HIGH, 3.3);
    tcan1463q1_simulator_step(sim, 1000000);  // 1ms
    
    OperatingMode mode = tcan1463q1_simulator_get_mode(sim);
    printf("   Current mode: %d\n", mode);
    printf("   Mode change events: %d\n\n", mode_change_count);
    
    printf("4. Simulating TXD dominant timeout fault...\n");
    tcan1463q1_simulator_set_pin(sim, PIN_TXD, PIN_STATE_LOW, 0.0);
    tcan1463q1_simulator_step(sim, 3000000);  // 3ms (exceeds TXDDTO)
    
    bool pwron, wakerq, wakesr, uvsup, uvcc, uvio, cbf;
    bool txdclp, txddto, txdrxd, candom, tsd;
    tcan1463q1_simulator_get_flags(sim, &pwron, &wakerq, &wakesr,
                                    &uvsup, &uvcc, &uvio, &cbf,
                                    &txdclp, &txddto, &txdrxd, &candom, &tsd);
    
    printf("   TXDDTO flag: %d\n", txddto);
    printf("   Fault events: %d\n\n", fault_count);
    
    printf("5. Transitioning to Silent mode...\n");
    tcan1463q1_simulator_set_pin(sim, PIN_EN, PIN_STATE_LOW, 0.0);
    tcan1463q1_simulator_step(sim, 1000000);  // 1ms
    
    mode = tcan1463q1_simulator_get_mode(sim);
    printf("   Current mode: %d\n", mode);
    printf("   Total mode change events: %d\n\n", mode_change_count);
    
    printf("6. Unregistering callbacks...\n");
    bool unreg1 = tcan1463q1_simulator_unregister_callback(
        sim, EVENT_MODE_CHANGE, on_mode_change);
    bool unreg2 = tcan1463q1_simulator_unregister_callback(
        sim, EVENT_FAULT_DETECTED, on_fault_detected);
    
    if (unreg1 && unreg2) {
        printf("   Callbacks unregistered successfully\n\n");
    }
    
    printf("7. Destroying simulator...\n");
    tcan1463q1_simulator_destroy(sim);
    printf("   Simulator destroyed\n\n");
    
    printf("=== Example completed ===\n");
    printf("Total mode changes: %d\n", mode_change_count);
    printf("Total faults: %d\n", fault_count);
    
    return 0;
}
