#ifndef TCAN1463Q1_SCENARIO_H
#define TCAN1463Q1_SCENARIO_H

#include "tcan1463q1_simulator.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Scenario action types
 */
typedef enum {
    ACTION_SET_PIN,           // Set a pin value
    ACTION_WAIT,              // Wait for specified time
    ACTION_WAIT_UNTIL,        // Wait until condition is met
    ACTION_CHECK_PIN,         // Check pin value matches expected
    ACTION_CHECK_MODE,        // Check operating mode
    ACTION_CHECK_FLAG,        // Check status flag
    ACTION_CONFIGURE,         // Configure simulator parameters
    ACTION_COMMENT            // Comment/documentation (no-op)
} ScenarioActionType;

/**
 * Flag type for checking
 */
typedef enum {
    FLAG_PWRON,
    FLAG_WAKERQ,
    FLAG_WAKESR,
    FLAG_UVSUP,
    FLAG_UVCC,
    FLAG_UVIO,
    FLAG_CBF,
    FLAG_TXDCLP,
    FLAG_TXDDTO,
    FLAG_TXDRXD,
    FLAG_CANDOM,
    FLAG_TSD
} FlagType;

/**
 * Scenario action structure
 */
typedef struct {
    ScenarioActionType type;
    const char* description;  // Human-readable description
    
    union {
        // ACTION_SET_PIN
        struct {
            PinType pin;
            PinState state;
            double voltage;
        } set_pin;
        
        // ACTION_WAIT
        struct {
            uint64_t duration_ns;
        } wait;
        
        // ACTION_WAIT_UNTIL
        struct {
            SimulationCondition condition;
            void* user_data;
            uint64_t timeout_ns;
        } wait_until;
        
        // ACTION_CHECK_PIN
        struct {
            PinType pin;
            PinState expected_state;
            double expected_voltage;
            double voltage_tolerance;
        } check_pin;
        
        // ACTION_CHECK_MODE
        struct {
            OperatingMode expected_mode;
        } check_mode;
        
        // ACTION_CHECK_FLAG
        struct {
            FlagType flag;
            bool expected_value;
        } check_flag;
        
        // ACTION_CONFIGURE
        struct {
            double vsup;
            double vcc;
            double vio;
            double tj_temperature;
            double rl_resistance;
            double cl_capacitance;
        } configure;
    } data;
} ScenarioAction;

/**
 * Scenario structure
 */
typedef struct {
    const char* name;
    const char* description;
    ScenarioAction* actions;
    size_t action_count;
    size_t current_action;
    bool stop_on_error;
} Scenario;

/**
 * Scenario execution result
 */
typedef struct {
    bool success;
    size_t actions_executed;
    size_t actions_passed;
    size_t actions_failed;
    const char* error_message;
    size_t failed_action_index;
} ScenarioResult;

// Scenario creation and management
Scenario* tcan1463q1_scenario_create(const char* name, const char* description);
void tcan1463q1_scenario_destroy(Scenario* scenario);
void tcan1463q1_scenario_reset(Scenario* scenario);

// Add actions to scenario
bool tcan1463q1_scenario_add_set_pin(Scenario* scenario, const char* description,
                                      PinType pin, PinState state, double voltage);
bool tcan1463q1_scenario_add_wait(Scenario* scenario, const char* description,
                                   uint64_t duration_ns);
bool tcan1463q1_scenario_add_wait_until(Scenario* scenario, const char* description,
                                         SimulationCondition condition, void* user_data,
                                         uint64_t timeout_ns);
bool tcan1463q1_scenario_add_check_pin(Scenario* scenario, const char* description,
                                        PinType pin, PinState expected_state,
                                        double expected_voltage, double tolerance);
bool tcan1463q1_scenario_add_check_mode(Scenario* scenario, const char* description,
                                         OperatingMode expected_mode);
bool tcan1463q1_scenario_add_check_flag(Scenario* scenario, const char* description,
                                         FlagType flag, bool expected_value);
bool tcan1463q1_scenario_add_configure(Scenario* scenario, const char* description,
                                        double vsup, double vcc, double vio,
                                        double tj_temperature, double rl_resistance,
                                        double cl_capacitance);
bool tcan1463q1_scenario_add_comment(Scenario* scenario, const char* comment);

// Scenario execution
ScenarioResult tcan1463q1_scenario_execute(Scenario* scenario, TCAN1463Q1Simulator* sim);
ScenarioResult tcan1463q1_scenario_execute_step(Scenario* scenario, TCAN1463Q1Simulator* sim);

// Scenario utilities
void tcan1463q1_scenario_print(const Scenario* scenario);
void tcan1463q1_scenario_result_print(const ScenarioResult* result);

// Pre-defined scenario builders
Scenario* tcan1463q1_scenario_power_up_sequence(void);
Scenario* tcan1463q1_scenario_normal_to_sleep(void);
Scenario* tcan1463q1_scenario_sleep_to_normal_via_wup(void);
Scenario* tcan1463q1_scenario_can_message_transmission(void);
Scenario* tcan1463q1_scenario_fault_detection_txd_timeout(void);
Scenario* tcan1463q1_scenario_undervoltage_recovery(void);

#ifdef __cplusplus
}
#endif

#endif // TCAN1463Q1_SCENARIO_H
