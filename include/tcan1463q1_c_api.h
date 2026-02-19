#ifndef TCAN1463Q1_C_API_H
#define TCAN1463Q1_C_API_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file tcan1463q1_c_api.h
 * @brief C-compatible API for TCAN1463-Q1 Simulator
 * 
 * This header provides a pure C interface to the TCAN1463-Q1 simulator.
 * It uses opaque handles and error codes for C compatibility.
 */

/* ========================================================================
 * Opaque Handle Types
 * ======================================================================== */

/**
 * @brief Opaque handle to a TCAN1463-Q1 simulator instance
 * 
 * This handle is used to reference a simulator instance in all C API calls.
 * The actual structure is hidden from C users.
 */
typedef struct TCAN1463Q1SimulatorHandle* TCAN1463Q1SimHandle;

/**
 * @brief Opaque handle to a simulator snapshot
 * 
 * Used for saving and restoring simulator state.
 */
typedef struct TCAN1463Q1SnapshotHandle* TCAN1463Q1SnapHandle;

/* ========================================================================
 * Error Code Enumeration
 * ======================================================================== */

/**
 * @brief Error codes returned by C API functions
 */
typedef enum {
    TCAN_SUCCESS = 0,                    /**< Operation completed successfully */
    TCAN_ERROR_INVALID_HANDLE = -1,      /**< Invalid simulator handle */
    TCAN_ERROR_INVALID_PIN = -2,         /**< Invalid pin type */
    TCAN_ERROR_INVALID_VOLTAGE = -3,     /**< Voltage out of valid range */
    TCAN_ERROR_INVALID_MODE = -4,        /**< Invalid operating mode */
    TCAN_ERROR_INVALID_PARAMETER = -5,   /**< Invalid parameter value */
    TCAN_ERROR_OUT_OF_MEMORY = -6,       /**< Memory allocation failed */
    TCAN_ERROR_NULL_POINTER = -7,        /**< Null pointer argument */
    TCAN_ERROR_INVALID_STATE = -8,       /**< Operation not valid in current state */
    TCAN_ERROR_INVALID_SNAPSHOT = -9     /**< Invalid snapshot handle */
} TCAN_ErrorCode;

/* ========================================================================
 * Pin Types and States
 * ======================================================================== */

/**
 * @brief Pin type enumeration (matches PinType from main API)
 */
typedef enum {
    TCAN_PIN_TXD = 0,       /**< TXD input pin */
    TCAN_PIN_RXD = 1,       /**< RXD output pin */
    TCAN_PIN_EN = 2,        /**< EN input pin */
    TCAN_PIN_NSTB = 3,      /**< nSTB input pin */
    TCAN_PIN_NFAULT = 4,    /**< nFAULT output pin */
    TCAN_PIN_WAKE = 5,      /**< WAKE input pin */
    TCAN_PIN_INH = 6,       /**< INH output pin */
    TCAN_PIN_INH_MASK = 7,  /**< INH_MASK input pin */
    TCAN_PIN_CANH = 8,      /**< CANH bidirectional pin */
    TCAN_PIN_CANL = 9,      /**< CANL bidirectional pin */
    TCAN_PIN_VSUP = 10,     /**< VSUP power pin */
    TCAN_PIN_VCC = 11,      /**< VCC power pin */
    TCAN_PIN_VIO = 12,      /**< VIO power pin */
    TCAN_PIN_GND = 13       /**< GND ground pin */
} TCAN_PinType;

/**
 * @brief Pin state enumeration (matches PinState from main API)
 */
typedef enum {
    TCAN_PIN_STATE_LOW = 0,            /**< Logic low */
    TCAN_PIN_STATE_HIGH = 1,           /**< Logic high */
    TCAN_PIN_STATE_HIGH_IMPEDANCE = 2, /**< High impedance (tri-state) */
    TCAN_PIN_STATE_ANALOG = 3          /**< Analog voltage */
} TCAN_PinState;

/**
 * @brief Operating mode enumeration (matches OperatingMode from main API)
 */
typedef enum {
    TCAN_MODE_NORMAL = 0,      /**< Normal mode */
    TCAN_MODE_SILENT = 1,      /**< Silent mode */
    TCAN_MODE_STANDBY = 2,     /**< Standby mode */
    TCAN_MODE_GO_TO_SLEEP = 3, /**< Go-to-sleep mode */
    TCAN_MODE_SLEEP = 4,       /**< Sleep mode */
    TCAN_MODE_OFF = 5          /**< Off mode */
} TCAN_OperatingMode;

/**
 * @brief Pin value structure for batch operations
 */
typedef struct {
    TCAN_PinType pin;      /**< Pin type */
    TCAN_PinState state;   /**< Pin state */
    double voltage;        /**< Pin voltage (for analog pins) */
} TCAN_PinValue;

/**
 * @brief Timing parameters structure
 */
typedef struct {
    double tuv_ms;           /**< Undervoltage filter time (100-350ms) */
    double ttxddto_ms;       /**< TXD dominant timeout (1.2-3.8ms) */
    double tbusdom_ms;       /**< Bus dominant timeout (1.4-3.8ms) */
    double twk_filter_us;    /**< Wake-up filter time (0.5-1.8μs) */
    double twk_timeout_ms;   /**< Wake-up timeout (0.8-2.0ms) */
    double tsilence_s;       /**< Silence timeout (0.6-1.2s) */
} TCAN_TimingParameters;

/* ========================================================================
 * Core Simulator Functions
 * ======================================================================== */

/**
 * @brief Create a new simulator instance
 * 
 * @param[out] handle Pointer to receive the simulator handle
 * @return TCAN_SUCCESS on success, error code otherwise
 * 
 * @note The caller must call tcan_simulator_destroy() to free resources
 */
TCAN_ErrorCode tcan_simulator_create(TCAN1463Q1SimHandle* handle);

/**
 * @brief Destroy a simulator instance
 * 
 * @param[in] handle Simulator handle to destroy
 * @return TCAN_SUCCESS on success, error code otherwise
 * 
 * @note After this call, the handle is invalid and must not be used
 */
TCAN_ErrorCode tcan_simulator_destroy(TCAN1463Q1SimHandle handle);

/**
 * @brief Reset simulator to initial state
 * 
 * @param[in] handle Simulator handle
 * @return TCAN_SUCCESS on success, error code otherwise
 */
TCAN_ErrorCode tcan_simulator_reset(TCAN1463Q1SimHandle handle);

/* ========================================================================
 * Pin I/O Functions
 * ======================================================================== */

/**
 * @brief Set pin value (for input pins)
 * 
 * @param[in] handle Simulator handle
 * @param[in] pin Pin type
 * @param[in] state Pin state
 * @param[in] voltage Pin voltage (for analog pins)
 * @return TCAN_SUCCESS on success, error code otherwise
 */
TCAN_ErrorCode tcan_simulator_set_pin(
    TCAN1463Q1SimHandle handle,
    TCAN_PinType pin,
    TCAN_PinState state,
    double voltage
);

/**
 * @brief Get pin value (for output pins)
 * 
 * @param[in] handle Simulator handle
 * @param[in] pin Pin type
 * @param[out] state Pointer to receive pin state
 * @param[out] voltage Pointer to receive pin voltage
 * @return TCAN_SUCCESS on success, error code otherwise
 */
TCAN_ErrorCode tcan_simulator_get_pin(
    TCAN1463Q1SimHandle handle,
    TCAN_PinType pin,
    TCAN_PinState* state,
    double* voltage
);

/**
 * @brief Set multiple pins at once
 * 
 * @param[in] handle Simulator handle
 * @param[in] values Array of pin values
 * @param[in] count Number of pins to set
 * @return TCAN_SUCCESS on success, error code otherwise
 */
TCAN_ErrorCode tcan_simulator_set_pins(
    TCAN1463Q1SimHandle handle,
    const TCAN_PinValue* values,
    size_t count
);

/**
 * @brief Get multiple pins at once
 * 
 * @param[in] handle Simulator handle
 * @param[in,out] values Array of pin values (pin field must be set)
 * @param[in] count Number of pins to get
 * @return TCAN_SUCCESS on success, error code otherwise
 */
TCAN_ErrorCode tcan_simulator_get_pins(
    TCAN1463Q1SimHandle handle,
    TCAN_PinValue* values,
    size_t count
);

/**
 * @brief Get pin metadata
 * 
 * @param[in] handle Simulator handle
 * @param[in] pin Pin type
 * @param[out] is_input Pointer to receive input flag (can be NULL)
 * @param[out] is_output Pointer to receive output flag (can be NULL)
 * @param[out] min_voltage Pointer to receive minimum voltage (can be NULL)
 * @param[out] max_voltage Pointer to receive maximum voltage (can be NULL)
 * @return TCAN_SUCCESS on success, error code otherwise
 */
TCAN_ErrorCode tcan_simulator_get_pin_info(
    TCAN1463Q1SimHandle handle,
    TCAN_PinType pin,
    int* is_input,
    int* is_output,
    double* min_voltage,
    double* max_voltage
);

/* ========================================================================
 * Simulation Control Functions
 * ======================================================================== */

/**
 * @brief Advance simulation time
 * 
 * @param[in] handle Simulator handle
 * @param[in] delta_ns Time delta in nanoseconds
 * @return TCAN_SUCCESS on success, error code otherwise
 */
TCAN_ErrorCode tcan_simulator_step(
    TCAN1463Q1SimHandle handle,
    uint64_t delta_ns
);

/**
 * @brief Simulation condition callback function type
 * 
 * @param[in] handle Simulator handle
 * @param[in] user_data User-provided data
 * @return Non-zero if condition is met, zero otherwise
 */
typedef int (*TCAN_SimulationCondition)(TCAN1463Q1SimHandle handle, void* user_data);

/**
 * @brief Run simulation until condition is met or timeout
 * 
 * @param[in] handle Simulator handle
 * @param[in] condition Condition callback function
 * @param[in] user_data User data passed to condition callback
 * @param[in] timeout_ns Timeout in nanoseconds
 * @return TCAN_SUCCESS on success, error code otherwise
 */
TCAN_ErrorCode tcan_simulator_run_until(
    TCAN1463Q1SimHandle handle,
    TCAN_SimulationCondition condition,
    void* user_data,
    uint64_t timeout_ns
);

/* ========================================================================
 * State Query Functions
 * ======================================================================== */

/**
 * @brief Get current operating mode
 * 
 * @param[in] handle Simulator handle
 * @param[out] mode Pointer to receive operating mode
 * @return TCAN_SUCCESS on success, error code otherwise
 */
TCAN_ErrorCode tcan_simulator_get_mode(
    TCAN1463Q1SimHandle handle,
    TCAN_OperatingMode* mode
);

/**
 * @brief Get status flags
 * 
 * @param[in] handle Simulator handle
 * @param[out] pwron PWRON flag (can be NULL)
 * @param[out] wakerq WAKERQ flag (can be NULL)
 * @param[out] wakesr WAKESR flag (can be NULL)
 * @param[out] uvsup UVSUP flag (can be NULL)
 * @param[out] uvcc UVCC flag (can be NULL)
 * @param[out] uvio UVIO flag (can be NULL)
 * @param[out] cbf CBF flag (can be NULL)
 * @param[out] txdclp TXDCLP flag (can be NULL)
 * @param[out] txddto TXDDTO flag (can be NULL)
 * @param[out] txdrxd TXDRXD flag (can be NULL)
 * @param[out] candom CANDOM flag (can be NULL)
 * @param[out] tsd TSD flag (can be NULL)
 * @return TCAN_SUCCESS on success, error code otherwise
 */
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
);

/* ========================================================================
 * Configuration Functions
 * ======================================================================== */

/**
 * @brief Configure simulator parameters
 * 
 * @param[in] handle Simulator handle
 * @param[in] vsup VSUP voltage
 * @param[in] vcc VCC voltage
 * @param[in] vio VIO voltage
 * @param[in] tj_temperature Junction temperature
 * @param[in] rl_resistance Bus load resistance
 * @param[in] cl_capacitance Bus load capacitance
 * @return TCAN_SUCCESS on success, error code otherwise
 */
TCAN_ErrorCode tcan_simulator_configure(
    TCAN1463Q1SimHandle handle,
    double vsup,
    double vcc,
    double vio,
    double tj_temperature,
    double rl_resistance,
    double cl_capacitance
);

/**
 * @brief Set supply voltages
 * 
 * @param[in] handle Simulator handle
 * @param[in] vsup VSUP voltage
 * @param[in] vcc VCC voltage
 * @param[in] vio VIO voltage
 * @return TCAN_SUCCESS on success, error code otherwise
 */
TCAN_ErrorCode tcan_simulator_set_supply_voltages(
    TCAN1463Q1SimHandle handle,
    double vsup,
    double vcc,
    double vio
);

/**
 * @brief Set junction temperature
 * 
 * @param[in] handle Simulator handle
 * @param[in] tj_temperature Junction temperature in Celsius
 * @return TCAN_SUCCESS on success, error code otherwise
 */
TCAN_ErrorCode tcan_simulator_set_temperature(
    TCAN1463Q1SimHandle handle,
    double tj_temperature
);

/**
 * @brief Set bus parameters
 * 
 * @param[in] handle Simulator handle
 * @param[in] rl_resistance Bus load resistance in ohms
 * @param[in] cl_capacitance Bus load capacitance in farads
 * @return TCAN_SUCCESS on success, error code otherwise
 */
TCAN_ErrorCode tcan_simulator_set_bus_parameters(
    TCAN1463Q1SimHandle handle,
    double rl_resistance,
    double cl_capacitance
);

/**
 * @brief Set timing parameters
 * 
 * @param[in] handle Simulator handle
 * @param[in] params Timing parameters structure
 * @return TCAN_SUCCESS on success, error code otherwise
 */
TCAN_ErrorCode tcan_simulator_set_timing_parameters(
    TCAN1463Q1SimHandle handle,
    const TCAN_TimingParameters* params
);

/**
 * @brief Get timing parameters
 * 
 * @param[in] handle Simulator handle
 * @param[out] params Pointer to receive timing parameters
 * @return TCAN_SUCCESS on success, error code otherwise
 */
TCAN_ErrorCode tcan_simulator_get_timing_parameters(
    TCAN1463Q1SimHandle handle,
    TCAN_TimingParameters* params
);

/* ========================================================================
 * Parameter Validation Functions
 * ======================================================================== */

/**
 * @brief Validate VSUP voltage
 * 
 * @param[in] vsup VSUP voltage to validate
 * @return TCAN_SUCCESS if valid, error code otherwise
 */
TCAN_ErrorCode tcan_simulator_validate_vsup(double vsup);

/**
 * @brief Validate VCC voltage
 * 
 * @param[in] vcc VCC voltage to validate
 * @return TCAN_SUCCESS if valid, error code otherwise
 */
TCAN_ErrorCode tcan_simulator_validate_vcc(double vcc);

/**
 * @brief Validate VIO voltage
 * 
 * @param[in] vio VIO voltage to validate
 * @return TCAN_SUCCESS if valid, error code otherwise
 */
TCAN_ErrorCode tcan_simulator_validate_vio(double vio);

/**
 * @brief Validate junction temperature
 * 
 * @param[in] tj_temperature Junction temperature to validate
 * @return TCAN_SUCCESS if valid, error code otherwise
 */
TCAN_ErrorCode tcan_simulator_validate_temperature(double tj_temperature);

/**
 * @brief Validate timing parameters
 * 
 * @param[in] params Timing parameters to validate
 * @return TCAN_SUCCESS if valid, error code otherwise
 */
TCAN_ErrorCode tcan_simulator_validate_timing_parameters(
    const TCAN_TimingParameters* params
);

/* ========================================================================
 * Snapshot Functions
 * ======================================================================== */

/**
 * @brief Create a snapshot of current simulator state
 * 
 * @param[in] handle Simulator handle
 * @param[out] snapshot Pointer to receive snapshot handle
 * @return TCAN_SUCCESS on success, error code otherwise
 * 
 * @note The caller must call tcan_simulator_snapshot_free() to free resources
 */
TCAN_ErrorCode tcan_simulator_snapshot(
    TCAN1463Q1SimHandle handle,
    TCAN1463Q1SnapHandle* snapshot
);

/**
 * @brief Restore simulator state from snapshot
 * 
 * @param[in] handle Simulator handle
 * @param[in] snapshot Snapshot handle
 * @return TCAN_SUCCESS on success, error code otherwise
 */
TCAN_ErrorCode tcan_simulator_restore(
    TCAN1463Q1SimHandle handle,
    TCAN1463Q1SnapHandle snapshot
);

/**
 * @brief Free snapshot resources
 * 
 * @param[in] snapshot Snapshot handle to free
 * @return TCAN_SUCCESS on success, error code otherwise
 * 
 * @note After this call, the snapshot handle is invalid
 */
TCAN_ErrorCode tcan_simulator_snapshot_free(TCAN1463Q1SnapHandle snapshot);

/* ========================================================================
 * Event Callback Functions
 * ======================================================================== */

/**
 * @brief Event type enumeration
 */
typedef enum {
    TCAN_EVENT_MODE_CHANGE = 0,   /**< Operating mode changed */
    TCAN_EVENT_FAULT_DETECTED = 1, /**< Fault detected */
    TCAN_EVENT_WAKE_UP = 2,        /**< Wake-up event */
    TCAN_EVENT_PIN_CHANGE = 3,     /**< Pin state changed */
    TCAN_EVENT_FLAG_CHANGE = 4     /**< Status flag changed */
} TCAN_EventType;

/**
 * @brief Event callback function type
 * 
 * @param[in] event_type Type of event
 * @param[in] timestamp Event timestamp in nanoseconds
 * @param[in] user_data User-provided data
 */
typedef void (*TCAN_EventCallback)(
    TCAN_EventType event_type,
    uint64_t timestamp,
    void* user_data
);

/**
 * @brief Register event callback
 * 
 * @param[in] handle Simulator handle
 * @param[in] event_type Event type to register for
 * @param[in] callback Callback function
 * @param[in] user_data User data passed to callback
 * @return TCAN_SUCCESS on success, error code otherwise
 */
TCAN_ErrorCode tcan_simulator_register_callback(
    TCAN1463Q1SimHandle handle,
    TCAN_EventType event_type,
    TCAN_EventCallback callback,
    void* user_data
);

/**
 * @brief Unregister event callback
 * 
 * @param[in] handle Simulator handle
 * @param[in] event_type Event type to unregister
 * @param[in] callback Callback function to remove
 * @return TCAN_SUCCESS on success, error code otherwise
 */
TCAN_ErrorCode tcan_simulator_unregister_callback(
    TCAN1463Q1SimHandle handle,
    TCAN_EventType event_type,
    TCAN_EventCallback callback
);

/* ========================================================================
 * Error Handling Functions
 * ======================================================================== */

/**
 * @brief Get error message string for error code
 * 
 * @param[in] error_code Error code
 * @return Human-readable error message (never NULL)
 */
const char* tcan_get_error_string(TCAN_ErrorCode error_code);

#ifdef __cplusplus
}
#endif

#endif /* TCAN1463Q1_C_API_H */
