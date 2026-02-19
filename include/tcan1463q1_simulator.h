#ifndef TCAN1463Q1_SIMULATOR_H
#define TCAN1463Q1_SIMULATOR_H

#include "tcan1463q1_types.h"
#include "inh_controller.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Configurable timing parameters structure
 */
typedef struct {
    double tuv_ms;           // Undervoltage filter time (100-350ms)
    double ttxddto_ms;       // TXD dominant timeout (1.2-3.8ms)
    double tbusdom_ms;       // Bus dominant timeout (1.4-3.8ms)
    double twk_filter_us;    // Wake-up filter time (0.5-1.8μs)
    double twk_timeout_ms;   // Wake-up timeout (0.8-2.0ms)
    double tsilence_s;       // Silence timeout (0.6-1.2s)
} TimingParameters;

/**
 * Simulator event types
 */
typedef enum {
    EVENT_MODE_CHANGE,
    EVENT_FAULT_DETECTED,
    EVENT_WAKE_UP,
    EVENT_PIN_CHANGE,
    EVENT_FLAG_CHANGE
} SimulatorEventType;

/**
 * Simulator event structure
 */
typedef struct {
    SimulatorEventType type;
    uint64_t timestamp;
    union {
        struct {
            OperatingMode old_mode;
            OperatingMode new_mode;
        } mode_change;
        struct {
            const char* fault_name;
            bool is_set;
        } fault;
        struct {
            PinType pin;
            PinState old_state;
            PinState new_state;
            double old_voltage;
            double new_voltage;
        } pin_change;
    } data;
} SimulatorEvent;

/**
 * Event callback function type
 */
typedef void (*EventCallback)(const SimulatorEvent* event, void* user_data);

/**
 * Event callback registration entry
 */
typedef struct EventCallbackEntry {
    EventCallback callback;
    void* user_data;
    struct EventCallbackEntry* next;
} EventCallbackEntry;

/**
 * Main simulator structure
 */
typedef struct {
    Pin pins[14];
    ModeState mode_state;
    CANTransceiver can_transceiver;
    PowerState power_state;
    FaultState fault_state;
    WakeState wake_state;
    BusBiasController bus_bias;
    INHController* inh_controller;
    TimingEngine timing;
    
    // Configuration
    double tj_temperature;
    double rl_resistance;
    double cl_capacitance;
    TimingParameters timing_params;
    
    // Event callbacks (linked lists for each event type)
    EventCallbackEntry* callbacks[5];  // One for each SimulatorEventType
} TCAN1463Q1Simulator;

/**
 * Pin value structure for batch operations
 */
typedef struct {
    PinType pin;
    PinState state;
    double voltage;
} PinValue;

/**
 * Simulation condition function type
 */
typedef bool (*SimulationCondition)(TCAN1463Q1Simulator* sim, void* user_data);

/**
 * Snapshot structure for state save/restore
 */
typedef struct {
    uint8_t* data;
    size_t size;
} SimulatorSnapshot;

// Core simulator functions
TCAN1463Q1Simulator* tcan1463q1_simulator_create(void);
void tcan1463q1_simulator_destroy(TCAN1463Q1Simulator* sim);
void tcan1463q1_simulator_reset(TCAN1463Q1Simulator* sim);

// Pin I/O functions
bool tcan1463q1_simulator_set_pin(TCAN1463Q1Simulator* sim, PinType pin, 
                                   PinState state, double voltage);
bool tcan1463q1_simulator_get_pin(TCAN1463Q1Simulator* sim, PinType pin, 
                                   PinState* state, double* voltage);
bool tcan1463q1_simulator_set_pins(TCAN1463Q1Simulator* sim, 
                                    const PinValue* values, size_t count);
bool tcan1463q1_simulator_get_pins(TCAN1463Q1Simulator* sim, 
                                    PinValue* values, size_t count);
bool tcan1463q1_simulator_get_pin_info(TCAN1463Q1Simulator* sim, PinType pin,
                                        bool* is_input, bool* is_output,
                                        double* min_voltage, double* max_voltage);

// Simulation control
void tcan1463q1_simulator_step(TCAN1463Q1Simulator* sim, uint64_t delta_ns);
bool tcan1463q1_simulator_run_until(TCAN1463Q1Simulator* sim,
                                     SimulationCondition condition,
                                     void* user_data, uint64_t timeout_ns);

// State query functions
OperatingMode tcan1463q1_simulator_get_mode(TCAN1463Q1Simulator* sim);
void tcan1463q1_simulator_get_flags(TCAN1463Q1Simulator* sim,
                                     bool* pwron, bool* wakerq, bool* wakesr,
                                     bool* uvsup, bool* uvcc, bool* uvio,
                                     bool* cbf, bool* txdclp, bool* txddto,
                                     bool* txdrxd, bool* candom, bool* tsd);

// Configuration functions
void tcan1463q1_simulator_configure(TCAN1463Q1Simulator* sim,
                                     double vsup, double vcc, double vio,
                                     double tj_temperature,
                                     double rl_resistance, double cl_capacitance);

// Parameter validation and setting functions
bool tcan1463q1_simulator_set_supply_voltages(TCAN1463Q1Simulator* sim,
                                                double vsup, double vcc, double vio);
bool tcan1463q1_simulator_set_temperature(TCAN1463Q1Simulator* sim, double tj_temperature);
bool tcan1463q1_simulator_set_bus_parameters(TCAN1463Q1Simulator* sim,
                                               double rl_resistance, double cl_capacitance);
bool tcan1463q1_simulator_set_timing_parameters(TCAN1463Q1Simulator* sim,
                                                  const TimingParameters* params);
bool tcan1463q1_simulator_get_timing_parameters(TCAN1463Q1Simulator* sim,
                                                  TimingParameters* params);

// Parameter validation functions
bool tcan1463q1_simulator_validate_vsup(double vsup);
bool tcan1463q1_simulator_validate_vcc(double vcc);
bool tcan1463q1_simulator_validate_vio(double vio);
bool tcan1463q1_simulator_validate_temperature(double tj_temperature);
bool tcan1463q1_simulator_validate_timing_parameters(const TimingParameters* params);

// Snapshot functions
SimulatorSnapshot* tcan1463q1_simulator_snapshot(TCAN1463Q1Simulator* sim);
bool tcan1463q1_simulator_restore(TCAN1463Q1Simulator* sim,
                                   const SimulatorSnapshot* snapshot);
void tcan1463q1_simulator_snapshot_free(SimulatorSnapshot* snapshot);

// Event callback functions
bool tcan1463q1_simulator_register_callback(TCAN1463Q1Simulator* sim,
                                             SimulatorEventType event_type,
                                             EventCallback callback,
                                             void* user_data);
bool tcan1463q1_simulator_unregister_callback(TCAN1463Q1Simulator* sim,
                                               SimulatorEventType event_type,
                                               EventCallback callback);

#ifdef __cplusplus
}
#endif

#endif // TCAN1463Q1_SIMULATOR_H
