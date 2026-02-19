#include "tcan1463q1_scenario.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Helper function to resize action array
static bool scenario_ensure_capacity(Scenario* scenario, size_t new_capacity) {
    if (!scenario) return false;
    
    ScenarioAction* new_actions = (ScenarioAction*)realloc(
        scenario->actions, 
        new_capacity * sizeof(ScenarioAction)
    );
    
    if (!new_actions) return false;
    
    scenario->actions = new_actions;
    return true;
}

Scenario* tcan1463q1_scenario_create(const char* name, const char* description) {
    Scenario* scenario = (Scenario*)malloc(sizeof(Scenario));
    if (!scenario) return NULL;
    
    scenario->name = name ? strdup(name) : NULL;
    scenario->description = description ? strdup(description) : NULL;
    scenario->actions = NULL;
    scenario->action_count = 0;
    scenario->current_action = 0;
    scenario->stop_on_error = true;
    
    return scenario;
}

void tcan1463q1_scenario_destroy(Scenario* scenario) {
    if (!scenario) return;
    
    if (scenario->name) free((void*)scenario->name);
    if (scenario->description) free((void*)scenario->description);
    
    // Free action descriptions
    for (size_t i = 0; i < scenario->action_count; i++) {
        if (scenario->actions[i].description) {
            free((void*)scenario->actions[i].description);
        }
    }
    
    if (scenario->actions) free(scenario->actions);
    free(scenario);
}

void tcan1463q1_scenario_reset(Scenario* scenario) {
    if (!scenario) return;
    scenario->current_action = 0;
}

bool tcan1463q1_scenario_add_set_pin(Scenario* scenario, const char* description,
                                      PinType pin, PinState state, double voltage) {
    if (!scenario) return false;
    
    if (!scenario_ensure_capacity(scenario, scenario->action_count + 1)) {
        return false;
    }
    
    ScenarioAction* action = &scenario->actions[scenario->action_count];
    action->type = ACTION_SET_PIN;
    action->description = description ? strdup(description) : NULL;
    action->data.set_pin.pin = pin;
    action->data.set_pin.state = state;
    action->data.set_pin.voltage = voltage;
    
    scenario->action_count++;
    return true;
}

bool tcan1463q1_scenario_add_wait(Scenario* scenario, const char* description,
                                   uint64_t duration_ns) {
    if (!scenario) return false;
    
    if (!scenario_ensure_capacity(scenario, scenario->action_count + 1)) {
        return false;
    }
    
    ScenarioAction* action = &scenario->actions[scenario->action_count];
    action->type = ACTION_WAIT;
    action->description = description ? strdup(description) : NULL;
    action->data.wait.duration_ns = duration_ns;
    
    scenario->action_count++;
    return true;
}

bool tcan1463q1_scenario_add_wait_until(Scenario* scenario, const char* description,
                                         SimulationCondition condition, void* user_data,
                                         uint64_t timeout_ns) {
    if (!scenario) return false;
    
    if (!scenario_ensure_capacity(scenario, scenario->action_count + 1)) {
        return false;
    }
    
    ScenarioAction* action = &scenario->actions[scenario->action_count];
    action->type = ACTION_WAIT_UNTIL;
    action->description = description ? strdup(description) : NULL;
    action->data.wait_until.condition = condition;
    action->data.wait_until.user_data = user_data;
    action->data.wait_until.timeout_ns = timeout_ns;
    
    scenario->action_count++;
    return true;
}

bool tcan1463q1_scenario_add_check_pin(Scenario* scenario, const char* description,
                                        PinType pin, PinState expected_state,
                                        double expected_voltage, double tolerance) {
    if (!scenario) return false;
    
    if (!scenario_ensure_capacity(scenario, scenario->action_count + 1)) {
        return false;
    }
    
    ScenarioAction* action = &scenario->actions[scenario->action_count];
    action->type = ACTION_CHECK_PIN;
    action->description = description ? strdup(description) : NULL;
    action->data.check_pin.pin = pin;
    action->data.check_pin.expected_state = expected_state;
    action->data.check_pin.expected_voltage = expected_voltage;
    action->data.check_pin.voltage_tolerance = tolerance;
    
    scenario->action_count++;
    return true;
}

bool tcan1463q1_scenario_add_check_mode(Scenario* scenario, const char* description,
                                         OperatingMode expected_mode) {
    if (!scenario) return false;
    
    if (!scenario_ensure_capacity(scenario, scenario->action_count + 1)) {
        return false;
    }
    
    ScenarioAction* action = &scenario->actions[scenario->action_count];
    action->type = ACTION_CHECK_MODE;
    action->description = description ? strdup(description) : NULL;
    action->data.check_mode.expected_mode = expected_mode;
    
    scenario->action_count++;
    return true;
}

bool tcan1463q1_scenario_add_check_flag(Scenario* scenario, const char* description,
                                         FlagType flag, bool expected_value) {
    if (!scenario) return false;
    
    if (!scenario_ensure_capacity(scenario, scenario->action_count + 1)) {
        return false;
    }
    
    ScenarioAction* action = &scenario->actions[scenario->action_count];
    action->type = ACTION_CHECK_FLAG;
    action->description = description ? strdup(description) : NULL;
    action->data.check_flag.flag = flag;
    action->data.check_flag.expected_value = expected_value;
    
    scenario->action_count++;
    return true;
}

bool tcan1463q1_scenario_add_configure(Scenario* scenario, const char* description,
                                        double vsup, double vcc, double vio,
                                        double tj_temperature, double rl_resistance,
                                        double cl_capacitance) {
    if (!scenario) return false;
    
    if (!scenario_ensure_capacity(scenario, scenario->action_count + 1)) {
        return false;
    }
    
    ScenarioAction* action = &scenario->actions[scenario->action_count];
    action->type = ACTION_CONFIGURE;
    action->description = description ? strdup(description) : NULL;
    action->data.configure.vsup = vsup;
    action->data.configure.vcc = vcc;
    action->data.configure.vio = vio;
    action->data.configure.tj_temperature = tj_temperature;
    action->data.configure.rl_resistance = rl_resistance;
    action->data.configure.cl_capacitance = cl_capacitance;
    
    scenario->action_count++;
    return true;
}

bool tcan1463q1_scenario_add_comment(Scenario* scenario, const char* comment) {
    if (!scenario) return false;
    
    if (!scenario_ensure_capacity(scenario, scenario->action_count + 1)) {
        return false;
    }
    
    ScenarioAction* action = &scenario->actions[scenario->action_count];
    action->type = ACTION_COMMENT;
    action->description = comment ? strdup(comment) : NULL;
    
    scenario->action_count++;
    return true;
}

ScenarioResult tcan1463q1_scenario_execute(Scenario* scenario, TCAN1463Q1Simulator* sim) {
    ScenarioResult result = {0};
    
    if (!scenario || !sim) {
        result.success = false;
        result.error_message = "Invalid scenario or simulator";
        return result;
    }
    
    tcan1463q1_scenario_reset(scenario);
    
    for (size_t i = 0; i < scenario->action_count; i++) {
        ScenarioResult step_result = tcan1463q1_scenario_execute_step(scenario, sim);
        
        result.actions_executed++;
        
        if (step_result.success) {
            result.actions_passed++;
        } else {
            result.actions_failed++;
            result.error_message = step_result.error_message;
            result.failed_action_index = i;
            
            if (scenario->stop_on_error) {
                result.success = false;
                return result;
            }
        }
    }
    
    result.success = (result.actions_failed == 0);
    return result;
}

ScenarioResult tcan1463q1_scenario_execute_step(Scenario* scenario, TCAN1463Q1Simulator* sim) {
    ScenarioResult result = {0};
    result.success = true;
    result.actions_executed = 1;
    result.actions_passed = 1;
    
    if (!scenario || !sim) {
        result.success = false;
        result.error_message = "Invalid scenario or simulator";
        return result;
    }
    
    if (scenario->current_action >= scenario->action_count) {
        result.success = false;
        result.error_message = "No more actions to execute";
        return result;
    }
    
    ScenarioAction* action = &scenario->actions[scenario->current_action];
    scenario->current_action++;
    
    // Execute action based on type
    switch (action->type) {
        case ACTION_SET_PIN:
            if (!tcan1463q1_simulator_set_pin(sim, 
                                              action->data.set_pin.pin,
                                              action->data.set_pin.state,
                                              action->data.set_pin.voltage)) {
                result.success = false;
                result.error_message = "Failed to set pin";
            }
            break;
            
        case ACTION_WAIT:
            tcan1463q1_simulator_step(sim, action->data.wait.duration_ns);
            break;
            
        case ACTION_WAIT_UNTIL:
            if (!tcan1463q1_simulator_run_until(sim,
                                                action->data.wait_until.condition,
                                                action->data.wait_until.user_data,
                                                action->data.wait_until.timeout_ns)) {
                result.success = false;
                result.error_message = "Wait until condition timeout";
            }
            break;
            
        case ACTION_CHECK_PIN: {
            PinState state;
            double voltage;
            if (!tcan1463q1_simulator_get_pin(sim, action->data.check_pin.pin, &state, &voltage)) {
                result.success = false;
                result.error_message = "Failed to get pin value";
            } else if (state != action->data.check_pin.expected_state) {
                result.success = false;
                result.error_message = "Pin state mismatch";
            }
            break;
        }
            
        case ACTION_CHECK_MODE: {
            OperatingMode mode = tcan1463q1_simulator_get_mode(sim);
            if (mode != action->data.check_mode.expected_mode) {
                result.success = false;
                result.error_message = "Operating mode mismatch";
            }
            break;
        }
            
        case ACTION_CHECK_FLAG: {
            bool flags[12] = {0};
            tcan1463q1_simulator_get_flags(sim,
                                           &flags[0], &flags[1], &flags[2],
                                           &flags[3], &flags[4], &flags[5],
                                           &flags[6], &flags[7], &flags[8],
                                           &flags[9], &flags[10], &flags[11]);
            
            if (flags[action->data.check_flag.flag] != action->data.check_flag.expected_value) {
                result.success = false;
                result.error_message = "Flag value mismatch";
            }
            break;
        }
            
        case ACTION_CONFIGURE:
            tcan1463q1_simulator_configure(sim,
                                           action->data.configure.vsup,
                                           action->data.configure.vcc,
                                           action->data.configure.vio,
                                           action->data.configure.tj_temperature,
                                           action->data.configure.rl_resistance,
                                           action->data.configure.cl_capacitance);
            break;
            
        case ACTION_COMMENT:
            // No-op
            break;
    }
    
    if (!result.success) {
        result.actions_passed = 0;
        result.actions_failed = 1;
    }
    
    return result;
}

void tcan1463q1_scenario_print(const Scenario* scenario) {
    if (!scenario) return;
    
    printf("Scenario: %s\n", scenario->name ? scenario->name : "(unnamed)");
    if (scenario->description) {
        printf("Description: %s\n", scenario->description);
    }
    printf("Actions: %zu\n", scenario->action_count);
    printf("\n");
    
    for (size_t i = 0; i < scenario->action_count; i++) {
        const ScenarioAction* action = &scenario->actions[i];
        printf("[%zu] ", i + 1);
        
        switch (action->type) {
            case ACTION_SET_PIN:
                printf("SET_PIN");
                break;
            case ACTION_WAIT:
                printf("WAIT");
                break;
            case ACTION_WAIT_UNTIL:
                printf("WAIT_UNTIL");
                break;
            case ACTION_CHECK_PIN:
                printf("CHECK_PIN");
                break;
            case ACTION_CHECK_MODE:
                printf("CHECK_MODE");
                break;
            case ACTION_CHECK_FLAG:
                printf("CHECK_FLAG");
                break;
            case ACTION_CONFIGURE:
                printf("CONFIGURE");
                break;
            case ACTION_COMMENT:
                printf("COMMENT");
                break;
        }
        
        if (action->description) {
            printf(": %s", action->description);
        }
        printf("\n");
    }
}

void tcan1463q1_scenario_result_print(const ScenarioResult* result) {
    if (!result) return;
    
    printf("\nScenario Result:\n");
    printf("  Success: %s\n", result->success ? "YES" : "NO");
    printf("  Actions executed: %zu\n", result->actions_executed);
    printf("  Actions passed: %zu\n", result->actions_passed);
    printf("  Actions failed: %zu\n", result->actions_failed);
    
    if (!result->success && result->error_message) {
        printf("  Error: %s (at action %zu)\n", result->error_message, result->failed_action_index + 1);
    }
}

// Pre-defined scenario: Power-up sequence
Scenario* tcan1463q1_scenario_power_up_sequence(void) {
    Scenario* scenario = tcan1463q1_scenario_create(
        "Power-Up Sequence",
        "Tests the power-up sequence from Off to Normal mode"
    );
    
    if (!scenario) return NULL;
    
    // Configure power supplies
    tcan1463q1_scenario_add_configure(scenario, "Set power supplies to valid levels",
                                       5.0, 5.0, 3.3, 25.0, 60.0, 100e-12);
    
    // Wait for power-up time
    tcan1463q1_scenario_add_wait(scenario, "Wait for power-up (tPWRUP)", 340000);
    
    // Set EN and nSTB high for Normal mode
    tcan1463q1_scenario_add_set_pin(scenario, "Set EN high", PIN_EN, PIN_STATE_HIGH, 3.3);
    tcan1463q1_scenario_add_set_pin(scenario, "Set nSTB high", PIN_NSTB, PIN_STATE_HIGH, 3.3);
    
    // Wait for mode transition
    tcan1463q1_scenario_add_wait(scenario, "Wait for mode transition", 200000);
    
    // Check we're in Normal mode
    tcan1463q1_scenario_add_check_mode(scenario, "Verify Normal mode", MODE_NORMAL);
    
    // Check PWRON flag is set
    tcan1463q1_scenario_add_check_flag(scenario, "Verify PWRON flag", FLAG_PWRON, true);
    
    return scenario;
}

// Pre-defined scenario: Normal to Sleep transition
Scenario* tcan1463q1_scenario_normal_to_sleep(void) {
    Scenario* scenario = tcan1463q1_scenario_create(
        "Normal to Sleep Transition",
        "Tests transition from Normal mode to Sleep mode"
    );
    
    if (!scenario) return NULL;
    
    tcan1463q1_scenario_add_comment(scenario, "Starting from Normal mode");
    
    // Set nSTB low to initiate Go-to-sleep
    tcan1463q1_scenario_add_set_pin(scenario, "Set nSTB low", PIN_NSTB, PIN_STATE_LOW, 0.0);
    
    // Check Go-to-sleep mode
    tcan1463q1_scenario_add_check_mode(scenario, "Verify Go-to-sleep mode", MODE_GO_TO_SLEEP);
    
    // Wait for silence timeout (tSILENCE)
    tcan1463q1_scenario_add_wait(scenario, "Wait for tSILENCE", 1000000000);
    
    // Check Sleep mode
    tcan1463q1_scenario_add_check_mode(scenario, "Verify Sleep mode", MODE_SLEEP);
    
    // Check INH pin is high-Z
    tcan1463q1_scenario_add_check_pin(scenario, "Verify INH is high-Z", 
                                       PIN_INH, PIN_STATE_HIGH_IMPEDANCE, 0.0, 0.1);
    
    return scenario;
}

// Placeholder implementations for other pre-defined scenarios
Scenario* tcan1463q1_scenario_sleep_to_normal_via_wup(void) {
    return tcan1463q1_scenario_create("Sleep to Normal via WUP", 
                                       "Wake-up from Sleep using WUP pattern");
}

Scenario* tcan1463q1_scenario_can_message_transmission(void) {
    return tcan1463q1_scenario_create("CAN Message Transmission",
                                       "Transmit a CAN message");
}

Scenario* tcan1463q1_scenario_fault_detection_txd_timeout(void) {
    return tcan1463q1_scenario_create("TXD Timeout Fault Detection",
                                       "Detect TXD dominant timeout fault");
}

Scenario* tcan1463q1_scenario_undervoltage_recovery(void) {
    return tcan1463q1_scenario_create("Undervoltage Recovery",
                                       "Test undervoltage detection and recovery");
}
