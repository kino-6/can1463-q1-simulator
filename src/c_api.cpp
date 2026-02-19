#include "tcan1463q1_c_api.h"
#include "tcan1463q1_simulator.h"
#include <stdlib.h>
#include <string.h>

// ========================================================================
// Internal Helper Functions
// ========================================================================

/**
 * @brief Convert C++ PinType to C TCAN_PinType
 */
static inline PinType c_to_cpp_pin_type(TCAN_PinType pin) {
    return (PinType)pin;
}

/**
 * @brief Convert C++ PinState to C TCAN_PinState
 */
static inline TCAN_PinState cpp_to_c_pin_state(PinState state) {
    return (TCAN_PinState)state;
}

/**
 * @brief Convert C TCAN_PinState to C++ PinState
 */
static inline PinState c_to_cpp_pin_state(TCAN_PinState state) {
    return (PinState)state;
}

/**
 * @brief Convert C++ OperatingMode to C TCAN_OperatingMode
 */
static inline TCAN_OperatingMode cpp_to_c_mode(OperatingMode mode) {
    return (TCAN_OperatingMode)mode;
}

// ========================================================================
// Core Simulator Functions
// ========================================================================

TCAN_ErrorCode tcan_simulator_create(TCAN1463Q1SimHandle* handle) {
    if (!handle) {
        return TCAN_ERROR_NULL_POINTER;
    }
    
    TCAN1463Q1Simulator* sim = tcan1463q1_simulator_create();
    if (!sim) {
        return TCAN_ERROR_OUT_OF_MEMORY;
    }
    
    *handle = (TCAN1463Q1SimHandle)sim;
    return TCAN_SUCCESS;
}

TCAN_ErrorCode tcan_simulator_destroy(TCAN1463Q1SimHandle handle) {
    if (!handle) {
        return TCAN_ERROR_INVALID_HANDLE;
    }
    
    TCAN1463Q1Simulator* sim = (TCAN1463Q1Simulator*)handle;
    tcan1463q1_simulator_destroy(sim);
    
    return TCAN_SUCCESS;
}

TCAN_ErrorCode tcan_simulator_reset(TCAN1463Q1SimHandle handle) {
    if (!handle) {
        return TCAN_ERROR_INVALID_HANDLE;
    }
    
    TCAN1463Q1Simulator* sim = (TCAN1463Q1Simulator*)handle;
    tcan1463q1_simulator_reset(sim);
    
    return TCAN_SUCCESS;
}

// ========================================================================
// Pin I/O Functions
// ========================================================================

TCAN_ErrorCode tcan_simulator_set_pin(
    TCAN1463Q1SimHandle handle,
    TCAN_PinType pin,
    TCAN_PinState state,
    double voltage
) {
    if (!handle) {
        return TCAN_ERROR_INVALID_HANDLE;
    }
    
    if (pin < TCAN_PIN_TXD || pin > TCAN_PIN_GND) {
        return TCAN_ERROR_INVALID_PIN;
    }
    
    TCAN1463Q1Simulator* sim = (TCAN1463Q1Simulator*)handle;
    PinType cpp_pin = c_to_cpp_pin_type(pin);
    PinState cpp_state = c_to_cpp_pin_state(state);
    
    bool success = tcan1463q1_simulator_set_pin(sim, cpp_pin, cpp_state, voltage);
    if (!success) {
        return TCAN_ERROR_INVALID_VOLTAGE;
    }
    
    return TCAN_SUCCESS;
}

TCAN_ErrorCode tcan_simulator_get_pin(
    TCAN1463Q1SimHandle handle,
    TCAN_PinType pin,
    TCAN_PinState* state,
    double* voltage
) {
    if (!handle) {
        return TCAN_ERROR_INVALID_HANDLE;
    }
    
    if (pin < TCAN_PIN_TXD || pin > TCAN_PIN_GND) {
        return TCAN_ERROR_INVALID_PIN;
    }
    
    TCAN1463Q1Simulator* sim = (TCAN1463Q1Simulator*)handle;
    PinType cpp_pin = c_to_cpp_pin_type(pin);
    PinState cpp_state;
    double cpp_voltage;
    
    bool success = tcan1463q1_simulator_get_pin(sim, cpp_pin, &cpp_state, &cpp_voltage);
    if (!success) {
        return TCAN_ERROR_INVALID_PIN;
    }
    
    if (state) {
        *state = cpp_to_c_pin_state(cpp_state);
    }
    if (voltage) {
        *voltage = cpp_voltage;
    }
    
    return TCAN_SUCCESS;
}

TCAN_ErrorCode tcan_simulator_set_pins(
    TCAN1463Q1SimHandle handle,
    const TCAN_PinValue* values,
    size_t count
) {
    if (!handle) {
        return TCAN_ERROR_INVALID_HANDLE;
    }
    
    if (!values && count > 0) {
        return TCAN_ERROR_NULL_POINTER;
    }
    
    TCAN1463Q1Simulator* sim = (TCAN1463Q1Simulator*)handle;
    
    // Convert C pin values to C++ pin values
    PinValue* cpp_values = (PinValue*)malloc(count * sizeof(PinValue));
    if (!cpp_values && count > 0) {
        return TCAN_ERROR_OUT_OF_MEMORY;
    }
    
    for (size_t i = 0; i < count; i++) {
        cpp_values[i].pin = c_to_cpp_pin_type(values[i].pin);
        cpp_values[i].state = c_to_cpp_pin_state(values[i].state);
        cpp_values[i].voltage = values[i].voltage;
    }
    
    bool success = tcan1463q1_simulator_set_pins(sim, cpp_values, count);
    free(cpp_values);
    
    if (!success) {
        return TCAN_ERROR_INVALID_PARAMETER;
    }
    
    return TCAN_SUCCESS;
}

TCAN_ErrorCode tcan_simulator_get_pins(
    TCAN1463Q1SimHandle handle,
    TCAN_PinValue* values,
    size_t count
) {
    if (!handle) {
        return TCAN_ERROR_INVALID_HANDLE;
    }
    
    if (!values && count > 0) {
        return TCAN_ERROR_NULL_POINTER;
    }
    
    TCAN1463Q1Simulator* sim = (TCAN1463Q1Simulator*)handle;
    
    // Convert C pin values to C++ pin values
    PinValue* cpp_values = (PinValue*)malloc(count * sizeof(PinValue));
    if (!cpp_values && count > 0) {
        return TCAN_ERROR_OUT_OF_MEMORY;
    }
    
    for (size_t i = 0; i < count; i++) {
        cpp_values[i].pin = c_to_cpp_pin_type(values[i].pin);
    }
    
    bool success = tcan1463q1_simulator_get_pins(sim, cpp_values, count);
    
    if (success) {
        for (size_t i = 0; i < count; i++) {
            values[i].state = cpp_to_c_pin_state(cpp_values[i].state);
            values[i].voltage = cpp_values[i].voltage;
        }
    }
    
    free(cpp_values);
    
    if (!success) {
        return TCAN_ERROR_INVALID_PARAMETER;
    }
    
    return TCAN_SUCCESS;
}

TCAN_ErrorCode tcan_simulator_get_pin_info(
    TCAN1463Q1SimHandle handle,
    TCAN_PinType pin,
    int* is_input,
    int* is_output,
    double* min_voltage,
    double* max_voltage
) {
    if (!handle) {
        return TCAN_ERROR_INVALID_HANDLE;
    }
    
    if (pin < TCAN_PIN_TXD || pin > TCAN_PIN_GND) {
        return TCAN_ERROR_INVALID_PIN;
    }
    
    TCAN1463Q1Simulator* sim = (TCAN1463Q1Simulator*)handle;
    PinType cpp_pin = c_to_cpp_pin_type(pin);
    bool cpp_is_input, cpp_is_output;
    double cpp_min_voltage, cpp_max_voltage;
    
    bool success = tcan1463q1_simulator_get_pin_info(
        sim, cpp_pin,
        &cpp_is_input, &cpp_is_output,
        &cpp_min_voltage, &cpp_max_voltage
    );
    
    if (!success) {
        return TCAN_ERROR_INVALID_PIN;
    }
    
    if (is_input) *is_input = cpp_is_input ? 1 : 0;
    if (is_output) *is_output = cpp_is_output ? 1 : 0;
    if (min_voltage) *min_voltage = cpp_min_voltage;
    if (max_voltage) *max_voltage = cpp_max_voltage;
    
    return TCAN_SUCCESS;
}

// ========================================================================
// Simulation Control Functions
// ========================================================================

TCAN_ErrorCode tcan_simulator_step(
    TCAN1463Q1SimHandle handle,
    uint64_t delta_ns
) {
    if (!handle) {
        return TCAN_ERROR_INVALID_HANDLE;
    }
    
    TCAN1463Q1Simulator* sim = (TCAN1463Q1Simulator*)handle;
    tcan1463q1_simulator_step(sim, delta_ns);
    
    return TCAN_SUCCESS;
}

// Wrapper for C++ condition callback
struct ConditionCallbackWrapper {
    TCAN_SimulationCondition c_callback;
    void* c_user_data;
};

static bool cpp_condition_wrapper(TCAN1463Q1Simulator* sim, void* user_data) {
    ConditionCallbackWrapper* wrapper = (ConditionCallbackWrapper*)user_data;
    TCAN1463Q1SimHandle handle = (TCAN1463Q1SimHandle)sim;
    return wrapper->c_callback(handle, wrapper->c_user_data) != 0;
}

TCAN_ErrorCode tcan_simulator_run_until(
    TCAN1463Q1SimHandle handle,
    TCAN_SimulationCondition condition,
    void* user_data,
    uint64_t timeout_ns
) {
    if (!handle) {
        return TCAN_ERROR_INVALID_HANDLE;
    }
    
    if (!condition) {
        return TCAN_ERROR_NULL_POINTER;
    }
    
    TCAN1463Q1Simulator* sim = (TCAN1463Q1Simulator*)handle;
    
    // Create wrapper for C callback
    ConditionCallbackWrapper wrapper;
    wrapper.c_callback = condition;
    wrapper.c_user_data = user_data;
    
    bool success = tcan1463q1_simulator_run_until(
        sim,
        cpp_condition_wrapper,
        &wrapper,
        timeout_ns
    );
    
    if (!success) {
        return TCAN_ERROR_INVALID_STATE;
    }
    
    return TCAN_SUCCESS;
}

// ========================================================================
// State Query Functions
// ========================================================================

TCAN_ErrorCode tcan_simulator_get_mode(
    TCAN1463Q1SimHandle handle,
    TCAN_OperatingMode* mode
) {
    if (!handle) {
        return TCAN_ERROR_INVALID_HANDLE;
    }
    
    if (!mode) {
        return TCAN_ERROR_NULL_POINTER;
    }
    
    TCAN1463Q1Simulator* sim = (TCAN1463Q1Simulator*)handle;
    OperatingMode cpp_mode = tcan1463q1_simulator_get_mode(sim);
    *mode = cpp_to_c_mode(cpp_mode);
    
    return TCAN_SUCCESS;
}

TCAN_ErrorCode tcan_simulator_get_flags(
    TCAN1463Q1SimHandle handle,
    int* pwron,
    int* wakerq,
    int* wakesr,
    int* uvsup,
    int* uvcc,
    int* uvio,
    int* cbf,
    int* txdclp,
    int* txddto,
    int* txdrxd,
    int* candom,
    int* tsd
) {
    if (!handle) {
        return TCAN_ERROR_INVALID_HANDLE;
    }
    
    TCAN1463Q1Simulator* sim = (TCAN1463Q1Simulator*)handle;
    bool cpp_pwron, cpp_wakerq, cpp_wakesr, cpp_uvsup, cpp_uvcc, cpp_uvio;
    bool cpp_cbf, cpp_txdclp, cpp_txddto, cpp_txdrxd, cpp_candom, cpp_tsd;
    
    tcan1463q1_simulator_get_flags(
        sim,
        &cpp_pwron, &cpp_wakerq, &cpp_wakesr,
        &cpp_uvsup, &cpp_uvcc, &cpp_uvio,
        &cpp_cbf, &cpp_txdclp, &cpp_txddto,
        &cpp_txdrxd, &cpp_candom, &cpp_tsd
    );
    
    if (pwron) *pwron = cpp_pwron ? 1 : 0;
    if (wakerq) *wakerq = cpp_wakerq ? 1 : 0;
    if (wakesr) *wakesr = cpp_wakesr ? 1 : 0;
    if (uvsup) *uvsup = cpp_uvsup ? 1 : 0;
    if (uvcc) *uvcc = cpp_uvcc ? 1 : 0;
    if (uvio) *uvio = cpp_uvio ? 1 : 0;
    if (cbf) *cbf = cpp_cbf ? 1 : 0;
    if (txdclp) *txdclp = cpp_txdclp ? 1 : 0;
    if (txddto) *txddto = cpp_txddto ? 1 : 0;
    if (txdrxd) *txdrxd = cpp_txdrxd ? 1 : 0;
    if (candom) *candom = cpp_candom ? 1 : 0;
    if (tsd) *tsd = cpp_tsd ? 1 : 0;
    
    return TCAN_SUCCESS;
}

// ========================================================================
// Configuration Functions
// ========================================================================

TCAN_ErrorCode tcan_simulator_configure(
    TCAN1463Q1SimHandle handle,
    double vsup,
    double vcc,
    double vio,
    double tj_temperature,
    double rl_resistance,
    double cl_capacitance
) {
    if (!handle) {
        return TCAN_ERROR_INVALID_HANDLE;
    }
    
    TCAN1463Q1Simulator* sim = (TCAN1463Q1Simulator*)handle;
    tcan1463q1_simulator_configure(sim, vsup, vcc, vio, tj_temperature, 
                                    rl_resistance, cl_capacitance);
    
    return TCAN_SUCCESS;
}

TCAN_ErrorCode tcan_simulator_set_supply_voltages(
    TCAN1463Q1SimHandle handle,
    double vsup,
    double vcc,
    double vio
) {
    if (!handle) {
        return TCAN_ERROR_INVALID_HANDLE;
    }
    
    TCAN1463Q1Simulator* sim = (TCAN1463Q1Simulator*)handle;
    bool success = tcan1463q1_simulator_set_supply_voltages(sim, vsup, vcc, vio);
    
    if (!success) {
        return TCAN_ERROR_INVALID_VOLTAGE;
    }
    
    return TCAN_SUCCESS;
}

TCAN_ErrorCode tcan_simulator_set_temperature(
    TCAN1463Q1SimHandle handle,
    double tj_temperature
) {
    if (!handle) {
        return TCAN_ERROR_INVALID_HANDLE;
    }
    
    TCAN1463Q1Simulator* sim = (TCAN1463Q1Simulator*)handle;
    bool success = tcan1463q1_simulator_set_temperature(sim, tj_temperature);
    
    if (!success) {
        return TCAN_ERROR_INVALID_PARAMETER;
    }
    
    return TCAN_SUCCESS;
}

TCAN_ErrorCode tcan_simulator_set_bus_parameters(
    TCAN1463Q1SimHandle handle,
    double rl_resistance,
    double cl_capacitance
) {
    if (!handle) {
        return TCAN_ERROR_INVALID_HANDLE;
    }
    
    TCAN1463Q1Simulator* sim = (TCAN1463Q1Simulator*)handle;
    bool success = tcan1463q1_simulator_set_bus_parameters(sim, rl_resistance, cl_capacitance);
    
    if (!success) {
        return TCAN_ERROR_INVALID_PARAMETER;
    }
    
    return TCAN_SUCCESS;
}

TCAN_ErrorCode tcan_simulator_set_timing_parameters(
    TCAN1463Q1SimHandle handle,
    const TCAN_TimingParameters* params
) {
    if (!handle) {
        return TCAN_ERROR_INVALID_HANDLE;
    }
    
    if (!params) {
        return TCAN_ERROR_NULL_POINTER;
    }
    
    TCAN1463Q1Simulator* sim = (TCAN1463Q1Simulator*)handle;
    
    // Convert C timing parameters to C++ timing parameters
    TimingParameters cpp_params;
    cpp_params.tuv_ms = params->tuv_ms;
    cpp_params.ttxddto_ms = params->ttxddto_ms;
    cpp_params.tbusdom_ms = params->tbusdom_ms;
    cpp_params.twk_filter_us = params->twk_filter_us;
    cpp_params.twk_timeout_ms = params->twk_timeout_ms;
    cpp_params.tsilence_s = params->tsilence_s;
    
    bool success = tcan1463q1_simulator_set_timing_parameters(sim, &cpp_params);
    
    if (!success) {
        return TCAN_ERROR_INVALID_PARAMETER;
    }
    
    return TCAN_SUCCESS;
}

TCAN_ErrorCode tcan_simulator_get_timing_parameters(
    TCAN1463Q1SimHandle handle,
    TCAN_TimingParameters* params
) {
    if (!handle) {
        return TCAN_ERROR_INVALID_HANDLE;
    }
    
    if (!params) {
        return TCAN_ERROR_NULL_POINTER;
    }
    
    TCAN1463Q1Simulator* sim = (TCAN1463Q1Simulator*)handle;
    TimingParameters cpp_params;
    
    bool success = tcan1463q1_simulator_get_timing_parameters(sim, &cpp_params);
    
    if (!success) {
        return TCAN_ERROR_INVALID_PARAMETER;
    }
    
    // Convert C++ timing parameters to C timing parameters
    params->tuv_ms = cpp_params.tuv_ms;
    params->ttxddto_ms = cpp_params.ttxddto_ms;
    params->tbusdom_ms = cpp_params.tbusdom_ms;
    params->twk_filter_us = cpp_params.twk_filter_us;
    params->twk_timeout_ms = cpp_params.twk_timeout_ms;
    params->tsilence_s = cpp_params.tsilence_s;
    
    return TCAN_SUCCESS;
}

// ========================================================================
// Parameter Validation Functions
// ========================================================================

TCAN_ErrorCode tcan_simulator_validate_vsup(double vsup) {
    bool valid = tcan1463q1_simulator_validate_vsup(vsup);
    return valid ? TCAN_SUCCESS : TCAN_ERROR_INVALID_VOLTAGE;
}

TCAN_ErrorCode tcan_simulator_validate_vcc(double vcc) {
    bool valid = tcan1463q1_simulator_validate_vcc(vcc);
    return valid ? TCAN_SUCCESS : TCAN_ERROR_INVALID_VOLTAGE;
}

TCAN_ErrorCode tcan_simulator_validate_vio(double vio) {
    bool valid = tcan1463q1_simulator_validate_vio(vio);
    return valid ? TCAN_SUCCESS : TCAN_ERROR_INVALID_VOLTAGE;
}

TCAN_ErrorCode tcan_simulator_validate_temperature(double tj_temperature) {
    bool valid = tcan1463q1_simulator_validate_temperature(tj_temperature);
    return valid ? TCAN_SUCCESS : TCAN_ERROR_INVALID_PARAMETER;
}

TCAN_ErrorCode tcan_simulator_validate_timing_parameters(
    const TCAN_TimingParameters* params
) {
    if (!params) {
        return TCAN_ERROR_NULL_POINTER;
    }
    
    // Convert C timing parameters to C++ timing parameters
    TimingParameters cpp_params;
    cpp_params.tuv_ms = params->tuv_ms;
    cpp_params.ttxddto_ms = params->ttxddto_ms;
    cpp_params.tbusdom_ms = params->tbusdom_ms;
    cpp_params.twk_filter_us = params->twk_filter_us;
    cpp_params.twk_timeout_ms = params->twk_timeout_ms;
    cpp_params.tsilence_s = params->tsilence_s;
    
    bool valid = tcan1463q1_simulator_validate_timing_parameters(&cpp_params);
    return valid ? TCAN_SUCCESS : TCAN_ERROR_INVALID_PARAMETER;
}

// ========================================================================
// Snapshot Functions
// ========================================================================

TCAN_ErrorCode tcan_simulator_snapshot(
    TCAN1463Q1SimHandle handle,
    TCAN1463Q1SnapHandle* snapshot
) {
    if (!handle) {
        return TCAN_ERROR_INVALID_HANDLE;
    }
    
    if (!snapshot) {
        return TCAN_ERROR_NULL_POINTER;
    }
    
    TCAN1463Q1Simulator* sim = (TCAN1463Q1Simulator*)handle;
    SimulatorSnapshot* cpp_snapshot = tcan1463q1_simulator_snapshot(sim);
    
    if (!cpp_snapshot) {
        return TCAN_ERROR_OUT_OF_MEMORY;
    }
    
    *snapshot = (TCAN1463Q1SnapHandle)cpp_snapshot;
    return TCAN_SUCCESS;
}

TCAN_ErrorCode tcan_simulator_restore(
    TCAN1463Q1SimHandle handle,
    TCAN1463Q1SnapHandle snapshot
) {
    if (!handle) {
        return TCAN_ERROR_INVALID_HANDLE;
    }
    
    if (!snapshot) {
        return TCAN_ERROR_INVALID_SNAPSHOT;
    }
    
    TCAN1463Q1Simulator* sim = (TCAN1463Q1Simulator*)handle;
    SimulatorSnapshot* cpp_snapshot = (SimulatorSnapshot*)snapshot;
    
    bool success = tcan1463q1_simulator_restore(sim, cpp_snapshot);
    
    if (!success) {
        return TCAN_ERROR_INVALID_SNAPSHOT;
    }
    
    return TCAN_SUCCESS;
}

TCAN_ErrorCode tcan_simulator_snapshot_free(TCAN1463Q1SnapHandle snapshot) {
    if (!snapshot) {
        return TCAN_ERROR_INVALID_SNAPSHOT;
    }
    
    SimulatorSnapshot* cpp_snapshot = (SimulatorSnapshot*)snapshot;
    tcan1463q1_simulator_snapshot_free(cpp_snapshot);
    
    return TCAN_SUCCESS;
}

// ========================================================================
// Event Callback Functions
// ========================================================================

TCAN_ErrorCode tcan_simulator_register_callback(
    TCAN1463Q1SimHandle handle,
    TCAN_EventType event_type,
    TCAN_EventCallback callback,
    void* user_data
) {
    if (!handle) {
        return TCAN_ERROR_INVALID_HANDLE;
    }
    
    // Event callback system not yet implemented
    return TCAN_ERROR_INVALID_STATE;
}

TCAN_ErrorCode tcan_simulator_unregister_callback(
    TCAN1463Q1SimHandle handle,
    TCAN_EventType event_type,
    TCAN_EventCallback callback
) {
    if (!handle) {
        return TCAN_ERROR_INVALID_HANDLE;
    }
    
    // Event callback system not yet implemented
    return TCAN_ERROR_INVALID_STATE;
}

// ========================================================================
// Error Handling Functions
// ========================================================================

const char* tcan_get_error_string(TCAN_ErrorCode error_code) {
    switch (error_code) {
        case TCAN_SUCCESS:
            return "Success";
        case TCAN_ERROR_INVALID_HANDLE:
            return "Invalid simulator handle";
        case TCAN_ERROR_INVALID_PIN:
            return "Invalid pin type";
        case TCAN_ERROR_INVALID_VOLTAGE:
            return "Voltage out of valid range";
        case TCAN_ERROR_INVALID_MODE:
            return "Invalid operating mode";
        case TCAN_ERROR_INVALID_PARAMETER:
            return "Invalid parameter value";
        case TCAN_ERROR_OUT_OF_MEMORY:
            return "Memory allocation failed";
        case TCAN_ERROR_NULL_POINTER:
            return "Null pointer argument";
        case TCAN_ERROR_INVALID_STATE:
            return "Operation not valid in current state";
        case TCAN_ERROR_INVALID_SNAPSHOT:
            return "Invalid snapshot handle";
        default:
            return "Unknown error";
    }
}
