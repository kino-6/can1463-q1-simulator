#ifndef TCAN1463Q1_TYPES_H
#define TCAN1463Q1_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Pin types for TCAN1463-Q1 IC
 */
typedef enum {
    PIN_TXD,      // Input: Transmit data input
    PIN_RXD,      // Output: Receive data output
    PIN_EN,       // Input: Enable pin
    PIN_NSTB,     // Input: Standby control (active low)
    PIN_NFAULT,   // Output: Fault indication (active low)
    PIN_WAKE,     // Input: Local wake-up input
    PIN_INH,      // Output: Inhibit output
    PIN_INH_MASK, // Input: INH mask control
    PIN_CANH,     // Bidirectional: CAN high line
    PIN_CANL,     // Bidirectional: CAN low line
    PIN_VSUP,     // Power: Supply voltage
    PIN_VCC,      // Power: Logic supply voltage
    PIN_VIO,      // Power: I/O supply voltage
    PIN_GND       // Ground
} PinType;

/**
 * Pin state enumeration
 */
typedef enum {
    PIN_STATE_LOW,
    PIN_STATE_HIGH,
    PIN_STATE_HIGH_IMPEDANCE,
    PIN_STATE_ANALOG
} PinState;

/**
 * Operating modes for TCAN1463-Q1
 */
typedef enum {
    MODE_NORMAL,      // EN=H, nSTB=H - Normal operation
    MODE_SILENT,      // EN=L, nSTB=H - Listen-only mode
    MODE_STANDBY,     // nSTB=L, WAKERQ set - Low power with wake capability
    MODE_GO_TO_SLEEP, // nSTB=L, WAKERQ cleared - Transitional state
    MODE_SLEEP,       // After tSILENCE timeout - Lowest power mode
    MODE_OFF          // VSUP < UVSUP - Power off state
} OperatingMode;

/**
 * CAN bus state
 */
typedef enum {
    BUS_STATE_DOMINANT,
    BUS_STATE_RECESSIVE,
    BUS_STATE_INDETERMINATE
} BusState;

/**
 * CAN transceiver state
 */
typedef enum {
    CAN_STATE_OFF,
    CAN_STATE_AUTONOMOUS_INACTIVE,
    CAN_STATE_AUTONOMOUS_ACTIVE,
    CAN_STATE_ACTIVE
} CANTransceiverState;

/**
 * Bus bias state
 */
typedef enum {
    BIAS_STATE_OFF,
    BIAS_STATE_AUTONOMOUS_INACTIVE,
    BIAS_STATE_AUTONOMOUS_ACTIVE,
    BIAS_STATE_ACTIVE
} BusBiasState;

/**
 * Wake-up pattern recognition state
 */
typedef enum {
    WUP_STATE_IDLE,
    WUP_STATE_FIRST_DOMINANT,
    WUP_STATE_RECESSIVE,
    WUP_STATE_SECOND_DOMINANT,
    WUP_STATE_COMPLETE
} WUPState;

/**
 * Pin structure
 */
typedef struct {
    PinState state;
    double voltage;
    bool is_input;
    bool is_output;
    double min_voltage;
    double max_voltage;
} Pin;

/**
 * Mode state structure
 */
typedef struct {
    OperatingMode current_mode;
    OperatingMode previous_mode;
    uint64_t mode_entry_time;
    bool wakerq_flag;
} ModeState;

/**
 * CAN transceiver structure
 */
typedef struct {
    CANTransceiverState state;
    bool driver_enabled;
    bool receiver_enabled;
    double canh_voltage;
    double canl_voltage;
    bool rxd_output;
    bool rxd_pending;           // True if RXD update is pending
    bool rxd_pending_value;     // Pending RXD value
    uint64_t rxd_update_time;   // Time when RXD should be updated
} CANTransceiver;

/**
 * Power state structure
 */
typedef struct {
    double vsup;
    double vcc;
    double vio;
    bool uvsup_flag;
    bool uvcc_flag;
    bool uvio_flag;
    bool pwron_flag;
    uint64_t uvcc_start_time;
    uint64_t uvio_start_time;
} PowerState;

/**
 * Fault state structure
 */
typedef struct {
    bool txdclp_flag;
    bool txddto_flag;
    bool txdrxd_flag;
    bool candom_flag;
    bool tsd_flag;
    bool cbf_flag;
    uint64_t txd_dominant_start;
    uint64_t bus_dominant_start;
    int cbf_transition_count;
} FaultState;

/**
 * Wake state structure
 */
typedef struct {
    bool wakerq_flag;
    bool wakesr_flag;
    bool wake_source_local;
    WUPState wup_state;
    uint64_t wup_phase_start;
    uint64_t wup_timeout_start;
    bool wake_pin_prev_state;
} WakeState;

/**
 * Bus bias controller structure
 */
typedef struct {
    BusBiasState state;
    uint64_t last_bus_activity;
} BusBiasController;

/**
 * Timing engine structure
 */
typedef struct {
    uint64_t current_time_ns;
    uint64_t last_update_ns;
} TimingEngine;

// Voltage thresholds
#define UVSUP_FALLING_MIN 3.5
#define UVSUP_FALLING_MAX 4.25
#define UVSUP_RISING_MIN 3.85
#define UVSUP_RISING_MAX 4.4

#define UVCC_FALLING_MIN 3.5
#define UVCC_FALLING_MAX 3.9
#define UVCC_RISING_MIN 4.1
#define UVCC_RISING_MAX 4.4

#define UVIO_FALLING_MIN 1.0
#define UVIO_FALLING_MAX 1.25
#define UVIO_RISING_MIN 1.4
#define UVIO_RISING_MAX 1.65

// Timing parameters (in appropriate units)
#define TUV_MIN_MS 100
#define TUV_MAX_MS 350

#define TTXDDTO_MIN_MS 1.2
#define TTXDDTO_MAX_MS 3.8
#define TBUSDOM_MIN_MS 1.4
#define TBUSDOM_MAX_MS 3.8
#define TCBF_US 2.5

#define TWK_FILTER_MIN_US 0.5
#define TWK_FILTER_MAX_US 1.8
#define TWK_TIMEOUT_MIN_MS 0.8
#define TWK_TIMEOUT_MAX_MS 2.0
#define TSILENCE_MIN_S 0.6
#define TSILENCE_MAX_S 1.2

#define TPROP_LOOP1_MIN_NS 100
#define TPROP_LOOP1_MAX_NS 190
#define TPROP_LOOP2_MIN_NS 110
#define TPROP_LOOP2_MAX_NS 190
#define TPWRUP_US 340
#define TMODE1_US 200
#define TBIAS_US 200
#define TINH_SLP_STB_US 100

#define TSDR_CELSIUS 165.0

#ifdef __cplusplus
}
#endif

#endif // TCAN1463Q1_TYPES_H
