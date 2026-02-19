#include "tcan1463q1_simulator.h"
#include "tcan1463q1_scenario.h"
#include <stdio.h>

int main() {
    printf("TCAN1463-Q1 Simulator - Scenario Example\n");
    printf("=========================================\n\n");
    
    // Create simulator
    TCAN1463Q1Simulator* sim = tcan1463q1_simulator_create();
    if (!sim) {
        printf("Failed to create simulator\n");
        return 1;
    }
    
    // Example 1: Power-up sequence
    printf("Example 1: Power-Up Sequence\n");
    printf("-----------------------------\n");
    Scenario* power_up = tcan1463q1_scenario_power_up_sequence();
    if (power_up) {
        tcan1463q1_scenario_print(power_up);
        ScenarioResult result = tcan1463q1_scenario_execute(power_up, sim);
        tcan1463q1_scenario_result_print(&result);
        tcan1463q1_scenario_destroy(power_up);
    }
    printf("\n");
    
    // Example 2: Normal to Sleep transition
    printf("Example 2: Normal to Sleep Transition\n");
    printf("--------------------------------------\n");
    Scenario* to_sleep = tcan1463q1_scenario_normal_to_sleep();
    if (to_sleep) {
        tcan1463q1_scenario_print(to_sleep);
        ScenarioResult result = tcan1463q1_scenario_execute(to_sleep, sim);
        tcan1463q1_scenario_result_print(&result);
        tcan1463q1_scenario_destroy(to_sleep);
    }
    printf("\n");
    
    // Example 3: Custom scenario
    printf("Example 3: Custom Scenario\n");
    printf("--------------------------\n");
    Scenario* custom = tcan1463q1_scenario_create(
        "Custom Test",
        "A custom test scenario demonstrating the API"
    );
    
    if (custom) {
        // Build custom scenario
        tcan1463q1_scenario_add_comment(custom, "=== Phase 1: Setup ===");
        tcan1463q1_scenario_add_configure(custom, "Configure power", 
                                          5.0, 5.0, 3.3, 25.0, 60.0, 100e-12);
        
        tcan1463q1_scenario_add_comment(custom, "=== Phase 2: Enter Normal Mode ===");
        tcan1463q1_scenario_add_set_pin(custom, "EN = HIGH", PIN_EN, PIN_STATE_HIGH, 3.3);
        tcan1463q1_scenario_add_set_pin(custom, "nSTB = HIGH", PIN_NSTB, PIN_STATE_HIGH, 3.3);
        tcan1463q1_scenario_add_wait(custom, "Wait 1ms", 1000000);
        
        tcan1463q1_scenario_add_comment(custom, "=== Phase 3: Verify State ===");
        tcan1463q1_scenario_add_check_mode(custom, "Check Normal mode", MODE_NORMAL);
        
        tcan1463q1_scenario_add_comment(custom, "=== Phase 4: CAN Communication ===");
        tcan1463q1_scenario_add_set_pin(custom, "TXD = LOW (dominant)", 
                                        PIN_TXD, PIN_STATE_LOW, 0.0);
        tcan1463q1_scenario_add_wait(custom, "Wait 10us", 10000);
        tcan1463q1_scenario_add_set_pin(custom, "TXD = HIGH (recessive)", 
                                        PIN_TXD, PIN_STATE_HIGH, 3.3);
        
        // Execute
        tcan1463q1_scenario_print(custom);
        ScenarioResult result = tcan1463q1_scenario_execute(custom, sim);
        tcan1463q1_scenario_result_print(&result);
        
        tcan1463q1_scenario_destroy(custom);
    }
    
    // Cleanup
    tcan1463q1_simulator_destroy(sim);
    
    printf("\nScenario examples completed!\n");
    return 0;
}
