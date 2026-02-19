#include "tcan1463q1_simulator.h"
#include "pin_manager.h"
#include "mode_controller.h"
#include "can_transceiver.h"
#include "power_monitor.h"
#include "fault_detector.h"
#include "wake_handler.h"
#include "bus_bias_controller.h"
#include "timing_engine.h"
#include "inh_controller.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

TCAN1463Q1Simulator* tcan1463q1_simulator_create(void) {
    TCAN1463Q1Simulator* sim = (TCAN1463Q1Simulator*)malloc(sizeof(TCAN1463Q1Simulator));
    if (sim) {
        memset(sim, 0, sizeof(TCAN1463Q1Simulator));
        
        // Allocate INH controller
        sim->inh_controller = (INHController*)malloc(sizeof(INHController));
        if (!sim->inh_controller) {
            free(sim);
            return NULL;
        }
        
        tcan1463q1_simulator_reset(sim);
    }
    return sim;
}

void tcan1463q1_simulator_destroy(TCAN1463Q1Simulator* sim) {
    if (sim) {
        // Free all event callbacks
        for (int i = 0; i < 5; i++) {
            EventCallbackEntry* entry = sim->callbacks[i];
            while (entry) {
                EventCallbackEntry* next = entry->next;
                free(entry);
                entry = next;
            }
        }
        
        if (sim->inh_controller) {
            free(sim->inh_controller);
        }
        free(sim);
    }
}

void tcan1463q1_simulator_reset(TCAN1463Q1Simulator* sim) {
    if (!sim) return;
    
    // Save INH controller pointer and callbacks
    INHController* inh_ctrl = sim->inh_controller;
    EventCallbackEntry* saved_callbacks[5];
    for (int i = 0; i < 5; i++) {
        saved_callbacks[i] = sim->callbacks[i];
    }
    
    // Initialize all state to default values
    memset(sim, 0, sizeof(TCAN1463Q1Simulator));
    
    // Restore INH controller pointer and callbacks
    sim->inh_controller = inh_ctrl;
    for (int i = 0; i < 5; i++) {
        sim->callbacks[i] = saved_callbacks[i];
    }
    
    // Initialize all components
    PinManager pin_mgr;
    pin_manager_init(&pin_mgr);
    memcpy(sim->pins, pin_mgr.pins, sizeof(sim->pins));
    
    mode_controller_init(&sim->mode_state);
    can_transceiver_init(&sim->can_transceiver);
    power_monitor_init(&sim->power_state);
    fault_detector_init(&sim->fault_state);
    wake_handler_init(&sim->wake_state);
    bus_bias_controller_init(&sim->bus_bias);
    timing_engine_init(&sim->timing);
    
    if (sim->inh_controller) {
        inh_controller_init(sim->inh_controller);
    }
    
    // Set initial mode to OFF
    sim->mode_state.current_mode = MODE_OFF;
    sim->mode_state.previous_mode = MODE_OFF;
    
    // Set default configuration
    sim->tj_temperature = 25.0;
    sim->rl_resistance = 60.0;
    sim->cl_capacitance = 100e-12;
    
    // Set default timing parameters (use middle of valid ranges)
    sim->timing_params.tuv_ms = (TUV_MIN_MS + TUV_MAX_MS) / 2.0;
    sim->timing_params.ttxddto_ms = (TTXDDTO_MIN_MS + TTXDDTO_MAX_MS) / 2.0;
    sim->timing_params.tbusdom_ms = (TBUSDOM_MIN_MS + TBUSDOM_MAX_MS) / 2.0;
    sim->timing_params.twk_filter_us = (TWK_FILTER_MIN_US + TWK_FILTER_MAX_US) / 2.0;
    sim->timing_params.twk_timeout_ms = (TWK_TIMEOUT_MIN_MS + TWK_TIMEOUT_MAX_MS) / 2.0;
    sim->timing_params.tsilence_s = (TSILENCE_MIN_S + TSILENCE_MAX_S) / 2.0;
}

bool tcan1463q1_simulator_set_pin(TCAN1463Q1Simulator* sim, PinType pin,
                                   PinState state, double voltage) {
    if (!sim) return false;
    
    // Validate pin index
    if (pin < 0 || pin >= 14) return false;
    
    // Set pin value using pin manager logic
    return pin_set_value(&sim->pins[pin], state, voltage);
}

bool tcan1463q1_simulator_get_pin(TCAN1463Q1Simulator* sim, PinType pin,
                                   PinState* state, double* voltage) {
    if (!sim) return false;
    
    // Validate pin index
    if (pin < 0 || pin >= 14) return false;
    
    // Get pin value
    return pin_get_value(&sim->pins[pin], state, voltage);
}

bool tcan1463q1_simulator_set_pins(TCAN1463Q1Simulator* sim,
                                    const PinValue* values, size_t count) {
    if (!sim || !values) return false;
    
    // Set multiple pins at once
    bool all_success = true;
    for (size_t i = 0; i < count; i++) {
        bool success = tcan1463q1_simulator_set_pin(
            sim, 
            values[i].pin, 
            values[i].state, 
            values[i].voltage
        );
        if (!success) {
            all_success = false;
        }
    }
    
    return all_success;
}

bool tcan1463q1_simulator_get_pins(TCAN1463Q1Simulator* sim,
                                    PinValue* values, size_t count) {
    if (!sim || !values) return false;
    
    // Get multiple pins at once
    bool all_success = true;
    for (size_t i = 0; i < count; i++) {
        bool success = tcan1463q1_simulator_get_pin(
            sim,
            values[i].pin,
            &values[i].state,
            &values[i].voltage
        );
        if (!success) {
            all_success = false;
        }
    }
    
    return all_success;
}

bool tcan1463q1_simulator_get_pin_info(TCAN1463Q1Simulator* sim, PinType pin,
                                        bool* is_input, bool* is_output,
                                        double* min_voltage, double* max_voltage) {
    if (!sim) return false;
    
    // Validate pin index
    if (pin < 0 || pin >= 14) return false;
    
    // Get pin metadata
    Pin* p = &sim->pins[pin];
    if (is_input) *is_input = p->is_input;
    if (is_output) *is_output = p->is_output;
    if (min_voltage) *min_voltage = p->min_voltage;
    if (max_voltage) *max_voltage = p->max_voltage;
    
    return true;
}

void tcan1463q1_simulator_step(TCAN1463Q1Simulator* sim, uint64_t delta_ns) {
    if (!sim) return;
    
    // Get current time BEFORE advancing (this is when pin changes occur)
    uint64_t time_before_step = timing_engine_get_time(&sim->timing);
    
    // Advance simulation time
    timing_engine_advance(&sim->timing, delta_ns);
    uint64_t current_time = timing_engine_get_time(&sim->timing);
    
    // Read input pin states
    PinState txd_state, en_state, nstb_state, wake_state, inh_mask_state;
    double txd_v, en_v, nstb_v, wake_v, inh_mask_v;
    pin_get_value(&sim->pins[PIN_TXD], &txd_state, &txd_v);
    pin_get_value(&sim->pins[PIN_EN], &en_state, &en_v);
    pin_get_value(&sim->pins[PIN_NSTB], &nstb_state, &nstb_v);
    pin_get_value(&sim->pins[PIN_WAKE], &wake_state, &wake_v);
    pin_get_value(&sim->pins[PIN_INH_MASK], &inh_mask_state, &inh_mask_v);
    
    bool txd_low = (txd_state == PIN_STATE_LOW);
    bool en_high = (en_state == PIN_STATE_HIGH);
    bool nstb_high = (nstb_state == PIN_STATE_HIGH);
    bool wake_pin_high = (wake_state == PIN_STATE_HIGH);
    bool inh_mask_high = (inh_mask_state == PIN_STATE_HIGH);
    
    // Read power supply voltages
    double vsup, vcc, vio;
    PinState vsup_state, vcc_state, vio_state;
    pin_get_value(&sim->pins[PIN_VSUP], &vsup_state, &vsup);
    pin_get_value(&sim->pins[PIN_VCC], &vcc_state, &vcc);
    pin_get_value(&sim->pins[PIN_VIO], &vio_state, &vio);
    
    // Update power monitor
    power_monitor_update(&sim->power_state, vsup, vcc, vio, current_time);
    bool vsup_valid = power_monitor_is_vsup_valid(&sim->power_state);
    
    // Update wake handler (using previous bus state for wake-up detection)
    double canh_voltage_prev, canl_voltage_prev;
    PinState canh_state_prev, canl_state_prev;
    pin_get_value(&sim->pins[PIN_CANH], &canh_state_prev, &canh_voltage_prev);
    pin_get_value(&sim->pins[PIN_CANL], &canl_state_prev, &canl_voltage_prev);
    double vdiff_prev = canh_voltage_prev - canl_voltage_prev;
    BusState bus_state_prev = can_transceiver_get_bus_state(vdiff_prev);
    
    wake_handler_update(&sim->wake_state, bus_state_prev, wake_pin_high, 
                       sim->mode_state.current_mode, current_time);
    bool wakerq = wake_handler_get_wakerq(&sim->wake_state);
    
    // Update mode controller
    OperatingMode old_mode = sim->mode_state.current_mode;
    OperatingMode new_mode = mode_controller_update(
        &sim->mode_state, en_high, nstb_high, vsup_valid, wakerq, current_time
    );
    
    // Clear flags on mode transition to Normal
    if (new_mode == MODE_NORMAL && old_mode != MODE_NORMAL) {
        power_monitor_clear_pwron_flag(&sim->power_state);
        wake_handler_clear_flags(&sim->wake_state);
    }
    
    // Update CAN transceiver state machine (before driving bus)
    can_transceiver_update(&sim->can_transceiver, new_mode, txd_low,
                          canh_voltage_prev, canl_voltage_prev, current_time);
    can_transceiver_update_state_machine(&sim->can_transceiver, new_mode,
                                         bus_state_prev, vsup_valid, current_time);
    
    // Update bus bias controller
    bus_bias_controller_update(&sim->bus_bias, sim->can_transceiver.state,
                               bus_state_prev, current_time);
    
    // Check for mode entry faults
    if (new_mode == MODE_NORMAL && old_mode != MODE_NORMAL) {
        fault_detector_check_txdclp(&sim->fault_state, txd_low, new_mode);
    }
    
    // Update INH controller
    if (sim->inh_controller) {
        inh_controller_update(sim->inh_controller, new_mode, inh_mask_high,
                            wakerq, current_time);
    }
    
    // === STEP 1: DRIVE BUS (based on TXD input) ===
    // CANH/CANL outputs (if driver is enabled)
    if (sim->can_transceiver.driver_enabled && 
        !fault_detector_should_disable_driver(&sim->fault_state)) {
        double canh_out, canl_out;
        can_transceiver_drive_bus(&sim->can_transceiver, txd_low, &canh_out, &canl_out);
        pin_set_value(&sim->pins[PIN_CANH], PIN_STATE_ANALOG, canh_out);
        pin_set_value(&sim->pins[PIN_CANL], PIN_STATE_ANALOG, canl_out);
    } else {
        // Apply bus bias if in appropriate state
        double canh_bias, canl_bias;
        bus_bias_controller_get_bias(&sim->bus_bias, vcc, &canh_bias, &canl_bias);
        
        if (sim->bus_bias.state != BIAS_STATE_OFF) {
            pin_set_value(&sim->pins[PIN_CANH], PIN_STATE_ANALOG, canh_bias);
            pin_set_value(&sim->pins[PIN_CANL], PIN_STATE_ANALOG, canl_bias);
        } else {
            pin_set_value(&sim->pins[PIN_CANH], PIN_STATE_HIGH_IMPEDANCE, 0.0);
            pin_set_value(&sim->pins[PIN_CANL], PIN_STATE_HIGH_IMPEDANCE, 0.0);
        }
    }
    
    // === STEP 2: READ BUS (after driving) ===
    double canh_voltage, canl_voltage;
    PinState canh_state, canl_state;
    pin_get_value(&sim->pins[PIN_CANH], &canh_state, &canh_voltage);
    pin_get_value(&sim->pins[PIN_CANL], &canl_state, &canl_voltage);
    
    // Get bus state from current voltages
    double vdiff = canh_voltage - canl_voltage;
    BusState bus_state = can_transceiver_get_bus_state(vdiff);
    
    // === STEP 3: UPDATE RXD (based on current bus state with propagation delay) ===
    // Update RXD output based on current bus state (respects propagation delay)
    // Use time_before_step for scheduling new updates, current_time for applying pending updates
    can_transceiver_update_rxd(&sim->can_transceiver, bus_state, current_time, time_before_step);
    bool rxd_high = sim->can_transceiver.rxd_output;
    
    // Update fault detector with current bus state
    fault_detector_update(&sim->fault_state, txd_low, !rxd_high, bus_state,
                         sim->tj_temperature, current_time, new_mode);
    
    // Update output pins
    
    // RXD output
    PinState rxd_state = rxd_high ? PIN_STATE_HIGH : PIN_STATE_LOW;
    pin_set_value(&sim->pins[PIN_RXD], rxd_state, rxd_high ? vio : 0.0);
    
    // nFAULT output
    bool nfault_low = fault_detector_get_nfault_state(&sim->fault_state) || wakerq;
    PinState nfault_state = nfault_low ? PIN_STATE_LOW : PIN_STATE_HIGH;
    pin_set_value(&sim->pins[PIN_NFAULT], nfault_state, nfault_low ? 0.0 : vio);
    
    // INH output
    if (sim->inh_controller) {
        PinState inh_state;
        double inh_voltage;
        inh_controller_get_pin_state(sim->inh_controller, &inh_state, &inh_voltage);
        pin_set_value(&sim->pins[PIN_INH], inh_state, inh_voltage);
    }
}

bool tcan1463q1_simulator_run_until(TCAN1463Q1Simulator* sim,
                                     SimulationCondition condition,
                                     void* user_data, uint64_t timeout_ns) {
    if (!sim || !condition) return false;
    
    uint64_t start_time = timing_engine_get_time(&sim->timing);
    uint64_t elapsed = 0;
    
    // Run simulation steps until condition is met or timeout
    while (elapsed < timeout_ns) {
        // Check condition
        if (condition(sim, user_data)) {
            return true;
        }
        
        // Step simulation by 1 microsecond
        uint64_t step_size = 1000; // 1 us
        tcan1463q1_simulator_step(sim, step_size);
        
        elapsed = timing_engine_get_time(&sim->timing) - start_time;
    }
    
    // Timeout reached, check condition one last time
    return condition(sim, user_data);
}

OperatingMode tcan1463q1_simulator_get_mode(TCAN1463Q1Simulator* sim) {
    if (!sim) return MODE_OFF;
    return sim->mode_state.current_mode;
}

void tcan1463q1_simulator_get_flags(TCAN1463Q1Simulator* sim,
                                     bool* pwron, bool* wakerq, bool* wakesr,
                                     bool* uvsup, bool* uvcc, bool* uvio,
                                     bool* cbf, bool* txdclp, bool* txddto,
                                     bool* txdrxd, bool* candom, bool* tsd) {
    if (!sim) return;
    
    if (pwron) *pwron = sim->power_state.pwron_flag;
    if (wakerq) *wakerq = sim->wake_state.wakerq_flag;
    if (wakesr) *wakesr = sim->wake_state.wakesr_flag;
    if (uvsup) *uvsup = sim->power_state.uvsup_flag;
    if (uvcc) *uvcc = sim->power_state.uvcc_flag;
    if (uvio) *uvio = sim->power_state.uvio_flag;
    if (cbf) *cbf = sim->fault_state.cbf_flag;
    if (txdclp) *txdclp = sim->fault_state.txdclp_flag;
    if (txddto) *txddto = sim->fault_state.txddto_flag;
    if (txdrxd) *txdrxd = sim->fault_state.txdrxd_flag;
    if (candom) *candom = sim->fault_state.candom_flag;
    if (tsd) *tsd = sim->fault_state.tsd_flag;
}

void tcan1463q1_simulator_configure(TCAN1463Q1Simulator* sim,
                                     double vsup, double vcc, double vio,
                                     double tj_temperature,
                                     double rl_resistance, double cl_capacitance) {
    if (!sim) return;
    
    sim->power_state.vsup = vsup;
    sim->power_state.vcc = vcc;
    sim->power_state.vio = vio;
    sim->tj_temperature = tj_temperature;
    sim->rl_resistance = rl_resistance;
    sim->cl_capacitance = cl_capacitance;
}

// Parameter validation functions
bool tcan1463q1_simulator_validate_vsup(double vsup) {
    // VSUP should be in reasonable range (0V to 40V for automotive)
    return vsup >= 0.0 && vsup <= 40.0;
}

bool tcan1463q1_simulator_validate_vcc(double vcc) {
    // VCC should be in reasonable range (0V to 6V)
    return vcc >= 0.0 && vcc <= 6.0;
}

bool tcan1463q1_simulator_validate_vio(double vio) {
    // VIO should be in reasonable range (0V to 5.5V)
    return vio >= 0.0 && vio <= 5.5;
}

bool tcan1463q1_simulator_validate_temperature(double tj_temperature) {
    // Junction temperature should be in reasonable range (-40°C to 200°C)
    return tj_temperature >= -40.0 && tj_temperature <= 200.0;
}

bool tcan1463q1_simulator_validate_timing_parameters(const TimingParameters* params) {
    if (!params) return false;
    
    // Validate each timing parameter against its specified range
    if (params->tuv_ms < TUV_MIN_MS || params->tuv_ms > TUV_MAX_MS) return false;
    if (params->ttxddto_ms < TTXDDTO_MIN_MS || params->ttxddto_ms > TTXDDTO_MAX_MS) return false;
    if (params->tbusdom_ms < TBUSDOM_MIN_MS || params->tbusdom_ms > TBUSDOM_MAX_MS) return false;
    if (params->twk_filter_us < TWK_FILTER_MIN_US || params->twk_filter_us > TWK_FILTER_MAX_US) return false;
    if (params->twk_timeout_ms < TWK_TIMEOUT_MIN_MS || params->twk_timeout_ms > TWK_TIMEOUT_MAX_MS) return false;
    if (params->tsilence_s < TSILENCE_MIN_S || params->tsilence_s > TSILENCE_MAX_S) return false;
    
    return true;
}

// Parameter setting functions with validation
bool tcan1463q1_simulator_set_supply_voltages(TCAN1463Q1Simulator* sim,
                                                double vsup, double vcc, double vio) {
    if (!sim) return false;
    
    // Validate all voltages
    if (!tcan1463q1_simulator_validate_vsup(vsup)) return false;
    if (!tcan1463q1_simulator_validate_vcc(vcc)) return false;
    if (!tcan1463q1_simulator_validate_vio(vio)) return false;
    
    // Set voltages
    sim->power_state.vsup = vsup;
    sim->power_state.vcc = vcc;
    sim->power_state.vio = vio;
    
    return true;
}

bool tcan1463q1_simulator_set_temperature(TCAN1463Q1Simulator* sim, double tj_temperature) {
    if (!sim) return false;
    
    // Validate temperature
    if (!tcan1463q1_simulator_validate_temperature(tj_temperature)) return false;
    
    // Set temperature
    sim->tj_temperature = tj_temperature;
    
    return true;
}

bool tcan1463q1_simulator_set_bus_parameters(TCAN1463Q1Simulator* sim,
                                               double rl_resistance, double cl_capacitance) {
    if (!sim) return false;
    
    // Validate bus parameters (must be positive)
    if (rl_resistance < 0.0 || cl_capacitance < 0.0) return false;
    
    // Set bus parameters
    sim->rl_resistance = rl_resistance;
    sim->cl_capacitance = cl_capacitance;
    
    return true;
}

bool tcan1463q1_simulator_set_timing_parameters(TCAN1463Q1Simulator* sim,
                                                  const TimingParameters* params) {
    if (!sim || !params) return false;
    
    // Validate timing parameters
    if (!tcan1463q1_simulator_validate_timing_parameters(params)) return false;
    
    // Set timing parameters
    sim->timing_params = *params;
    
    return true;
}

bool tcan1463q1_simulator_get_timing_parameters(TCAN1463Q1Simulator* sim,
                                                  TimingParameters* params) {
    if (!sim || !params) return false;
    
    // Get timing parameters
    *params = sim->timing_params;
    
    return true;
}

SimulatorSnapshot* tcan1463q1_simulator_snapshot(TCAN1463Q1Simulator* sim) {
    if (!sim) return NULL;
    
    SimulatorSnapshot* snapshot = (SimulatorSnapshot*)malloc(sizeof(SimulatorSnapshot));
    if (!snapshot) return NULL;
    
    // Allocate memory for simulator state (excluding INH controller pointer)
    snapshot->size = sizeof(TCAN1463Q1Simulator);
    snapshot->data = (uint8_t*)malloc(snapshot->size);
    if (!snapshot->data) {
        free(snapshot);
        return NULL;
    }
    
    // Copy simulator state
    memcpy(snapshot->data, sim, snapshot->size);
    
    return snapshot;
}

bool tcan1463q1_simulator_restore(TCAN1463Q1Simulator* sim,
                                   const SimulatorSnapshot* snapshot) {
    if (!sim || !snapshot || !snapshot->data) return false;
    
    // Verify snapshot size matches
    if (snapshot->size != sizeof(TCAN1463Q1Simulator)) return false;
    
    // Save INH controller pointer
    INHController* inh_ctrl = sim->inh_controller;
    
    // Restore simulator state
    memcpy(sim, snapshot->data, snapshot->size);
    
    // Restore INH controller pointer
    sim->inh_controller = inh_ctrl;
    
    return true;
}

void tcan1463q1_simulator_snapshot_free(SimulatorSnapshot* snapshot) {
    if (snapshot) {
        if (snapshot->data) {
            free(snapshot->data);
        }
        free(snapshot);
    }
}

bool tcan1463q1_simulator_register_callback(TCAN1463Q1Simulator* sim,
                                             SimulatorEventType event_type,
                                             EventCallback callback,
                                             void* user_data) {
    if (!sim || !callback) return false;
    if (event_type < 0 || event_type >= 5) return false;
    
    // Create new callback entry
    EventCallbackEntry* entry = (EventCallbackEntry*)malloc(sizeof(EventCallbackEntry));
    if (!entry) return false;
    
    entry->callback = callback;
    entry->user_data = user_data;
    entry->next = sim->callbacks[event_type];
    
    // Add to front of linked list
    sim->callbacks[event_type] = entry;
    
    return true;
}

bool tcan1463q1_simulator_unregister_callback(TCAN1463Q1Simulator* sim,
                                               SimulatorEventType event_type,
                                               EventCallback callback) {
    if (!sim || !callback) return false;
    if (event_type < 0 || event_type >= 5) return false;
    
    EventCallbackEntry** current = &sim->callbacks[event_type];
    
    while (*current) {
        if ((*current)->callback == callback) {
            EventCallbackEntry* to_remove = *current;
            *current = (*current)->next;
            free(to_remove);
            return true;
        }
        current = &(*current)->next;
    }
    
    return false;
}

// Helper function to fire events
static void fire_event(TCAN1463Q1Simulator* sim, const SimulatorEvent* event) {
    if (!sim || !event) return;
    if (event->type < 0 || event->type >= 5) return;
    
    EventCallbackEntry* entry = sim->callbacks[event->type];
    while (entry) {
        if (entry->callback) {
            entry->callback(event, entry->user_data);
        }
        entry = entry->next;
    }
}
