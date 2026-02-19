# Design Document: TCAN1463-Q1 Simulator

## Overview

TCAN1463-Q1 CANトランシーバーICシミュレーターは、実際のハードウェアICの動作を忠実に再現するソフトウェアシステムです。このシミュレーターは、C/C++で実装され、以下の主要コンポーネントで構成されます：

- **Pin Manager**: 全てのピンの状態を管理
- **Mode Controller**: 6つの動作モード間の遷移を制御
- **CAN Transceiver**: CAN/CAN FD通信をシミュレート
- **Power Monitor**: 電源電圧を監視し、アンダーボルテージを検出
- **Fault Detector**: 各種フォールト条件を検出
- **Wake Handler**: ウェイクアップイベントを処理
- **Bus Bias Controller**: CANバスのバイアス状態を管理
- **Timing Engine**: タイミング特性をシミュレート

## Architecture

### システム構成図

```
┌─────────────────────────────────────────────────────────────┐
│                    TCAN1463Q1Simulator                      │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │
│  │ Pin Manager  │  │Mode Controller│ │Power Monitor │    │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘    │
│         │                  │                  │             │
│  ┌──────┴──────────────────┴──────────────────┴───────┐   │
│  │              Timing Engine                          │   │
│  └──────┬──────────────────┬──────────────────┬───────┘   │
│         │                  │                  │             │
│  ┌──────┴───────┐  ┌──────┴───────┐  ┌──────┴───────┐    │
│  │CAN Transceiver│ │Fault Detector│  │Wake Handler  │    │
│  └──────────────┘  └──────────────┘  └──────────────┘    │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐ │
│  │          Bus Bias Controller                         │ │
│  └──────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### データフロー

1. **入力**: ユーザーがAPIを通じてピン値を設定
2. **処理**: Pin Managerが値を検証し、各コンポーネントに通知
3. **状態更新**: Mode Controller、Power Monitor等が状態を更新
4. **出力**: 出力ピンの値が更新され、ユーザーが読み取り可能

## Components and Interfaces

### 1. Pin Manager

**責務**: 全てのピンの状態を管理し、入出力を制御

**インターフェース**:
```c
typedef enum {
    PIN_TXD,      // Input
    PIN_RXD,      // Output
    PIN_EN,       // Input
    PIN_NSTB,     // Input
    PIN_NFAULT,   // Output
    PIN_WAKE,     // Input
    PIN_INH,      // Output
    PIN_INH_MASK, // Input
    PIN_CANH,     // Bidirectional
    PIN_CANL,     // Bidirectional
    PIN_VSUP,     // Power
    PIN_VCC,      // Power
    PIN_VIO,      // Power
    PIN_GND       // Ground
} PinType;

typedef enum {
    PIN_STATE_LOW,
    PIN_STATE_HIGH,
    PIN_STATE_HIGH_IMPEDANCE,
    PIN_STATE_ANALOG
} PinState;

typedef struct {
    PinState state;
    double voltage;  // For analog pins
    bool is_input;
    bool is_output;
} Pin;

// Set pin value (for input pins)
bool pin_manager_set_pin(PinType pin, PinState state, double voltage);

// Get pin value (for output pins)
bool pin_manager_get_pin(PinType pin, PinState* state, double* voltage);

// Validate pin value against voltage range
bool pin_manager_validate_pin(PinType pin, double voltage);
```

### 2. Mode Controller

**責務**: 動作モード間の遷移を制御

**インターフェース**:
```c
typedef enum {
    MODE_NORMAL,      // EN=H, nSTB=H
    MODE_SILENT,      // EN=L, nSTB=H
    MODE_STANDBY,     // nSTB=L, WAKERQ set
    MODE_GO_TO_SLEEP, // nSTB=L, WAKERQ cleared, transitional
    MODE_SLEEP,       // After tSILENCE timeout
    MODE_OFF          // VSUP < UVSUP
} OperatingMode;

typedef struct {
    OperatingMode current_mode;
    OperatingMode previous_mode;
    uint64_t mode_entry_time;  // Timestamp when entered current mode
    bool wakerq_flag;
} ModeState;

// Update mode based on pin states and conditions
OperatingMode mode_controller_update(
    bool en_high,
    bool nstb_high,
    bool vsup_valid,
    bool wakerq_set,
    uint64_t current_time
);

// Check if mode transition is valid
bool mode_controller_can_transition(OperatingMode from, OperatingMode to);

// Get current mode
OperatingMode mode_controller_get_mode(void);
```

### 3. CAN Transceiver

**責務**: CAN/CAN FD通信をシミュレート

**インターフェース**:
```c
typedef enum {
    BUS_STATE_DOMINANT,
    BUS_STATE_RECESSIVE,
    BUS_STATE_INDETERMINATE
} BusState;

typedef enum {
    CAN_STATE_OFF,
    CAN_STATE_AUTONOMOUS_INACTIVE,
    CAN_STATE_AUTONOMOUS_ACTIVE,
    CAN_STATE_ACTIVE
} CANTransceiverState;

typedef struct {
    CANTransceiverState state;
    bool driver_enabled;
    bool receiver_enabled;
    double canh_voltage;
    double canl_voltage;
    bool rxd_output;
} CANTransceiver;

// Update CAN transceiver based on mode and TXD input
void can_transceiver_update(
    OperatingMode mode,
    bool txd_low,
    double canh_voltage,
    double canl_voltage
);

// Get bus state from differential voltage
BusState can_transceiver_get_bus_state(double vdiff);

// Drive bus (set CANH/CANL voltages)
void can_transceiver_drive_bus(bool dominant, double* canh, double* canl);

// Update RXD output based on bus state
void can_transceiver_update_rxd(BusState bus_state);
```

### 4. Power Monitor

**責務**: 電源電圧を監視し、アンダーボルテージを検出

**インターフェース**:
```c
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

// Thresholds
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

#define TUV_MIN_MS 100
#define TUV_MAX_MS 350

// Update power state
void power_monitor_update(
    double vsup,
    double vcc,
    double vio,
    uint64_t current_time
);

// Check if supplies are valid
bool power_monitor_is_vsup_valid(void);
bool power_monitor_is_vcc_valid(void);
bool power_monitor_is_vio_valid(void);

// Get flags
bool power_monitor_get_uvsup_flag(void);
bool power_monitor_get_uvcc_flag(void);
bool power_monitor_get_uvio_flag(void);
bool power_monitor_get_pwron_flag(void);
```

### 5. Fault Detector

**責務**: 各種フォールト条件を検出

**インターフェース**:
```c
typedef struct {
    bool txdclp_flag;   // TXD clamped low
    bool txddto_flag;   // TXD dominant timeout
    bool txdrxd_flag;   // TXD/RXD short
    bool candom_flag;   // CAN bus dominant fault
    bool tsd_flag;      // Thermal shutdown
    bool cbf_flag;      // CAN bus fault
    uint64_t txd_dominant_start;
    uint64_t bus_dominant_start;
    int cbf_transition_count;
} FaultState;

// Timing parameters
#define TTXDDTO_MIN_MS 1.2
#define TTXDDTO_MAX_MS 3.8
#define TBUSDOM_MIN_MS 1.4
#define TBUSDOM_MAX_MS 3.8
#define TCBF_US 2.5

// Update fault detection
void fault_detector_update(
    bool txd_low,
    bool rxd_low,
    BusState bus_state,
    double tj_temperature,
    uint64_t current_time,
    OperatingMode mode
);

// Check for TXD clamped low before entering Normal mode
void fault_detector_check_txdclp(bool txd_low, OperatingMode entering_mode);

// Check for TXD dominant timeout
void fault_detector_check_txddto(bool txd_low, uint64_t current_time);

// Check for TXD/RXD short
void fault_detector_check_txdrxd(bool txd_low, bool rxd_low, uint64_t current_time);

// Check for CAN bus dominant fault
void fault_detector_check_candom(BusState bus_state, uint64_t current_time);

// Check for thermal shutdown
void fault_detector_check_tsd(double tj_temperature);

// Check for CAN bus fault (4 consecutive transitions)
void fault_detector_check_cbf(bool txd_transition, OperatingMode mode);

// Get fault flags
bool fault_detector_has_any_fault(void);
bool fault_detector_get_nfault_state(void);
```

### 6. Wake Handler

**責務**: ウェイクアップイベントを処理

**インターフェース**:
```c
typedef enum {
    WUP_STATE_IDLE,
    WUP_STATE_FIRST_DOMINANT,
    WUP_STATE_RECESSIVE,
    WUP_STATE_SECOND_DOMINANT,
    WUP_STATE_COMPLETE
} WUPState;

typedef struct {
    bool wakerq_flag;
    bool wakesr_flag;
    bool wake_source_local;  // true=local, false=remote
    WUPState wup_state;
    uint64_t wup_phase_start;
    uint64_t wup_timeout_start;
    bool wake_pin_prev_state;
} WakeState;

// Timing parameters
#define TWK_FILTER_MIN_US 0.5
#define TWK_FILTER_MAX_US 1.8
#define TWK_TIMEOUT_MIN_MS 0.8
#define TWK_TIMEOUT_MAX_MS 2.0
#define TSILENCE_MIN_S 0.6
#define TSILENCE_MAX_S 1.2

// Update wake handler
void wake_handler_update(
    BusState bus_state,
    bool wake_pin_high,
    OperatingMode mode,
    uint64_t current_time
);

// Process WUP pattern on CAN bus
void wake_handler_process_wup(BusState bus_state, uint64_t current_time);

// Process local wake-up from WAKE pin
void wake_handler_process_lwu(bool wake_pin_high, uint64_t current_time);

// Clear wake-up flags
void wake_handler_clear_flags(void);

// Get flags
bool wake_handler_get_wakerq(void);
bool wake_handler_get_wakesr(void);
bool wake_handler_is_local_wake(void);
```

### 7. Bus Bias Controller

**責務**: CANバスのバイアス状態を管理

**インターフェース**:
```c
typedef enum {
    BIAS_STATE_OFF,
    BIAS_STATE_AUTONOMOUS_INACTIVE,
    BIAS_STATE_AUTONOMOUS_ACTIVE,
    BIAS_STATE_ACTIVE
} BusBiasState;

typedef struct {
    BusBiasState state;
    uint64_t last_bus_activity;
} BusBiasController;

// Update bus bias state
void bus_bias_controller_update(
    CANTransceiverState can_state,
    BusState bus_state,
    uint64_t current_time
);

// Get bias voltages
void bus_bias_controller_get_bias(double* canh, double* canl);

// Check if bus is inactive for tSILENCE
bool bus_bias_controller_is_silence_timeout(uint64_t current_time);
```

### 8. Timing Engine

**責務**: タイミング特性をシミュレート

**インターフェース**:
```c
typedef struct {
    uint64_t current_time_ns;  // Nanoseconds
    uint64_t last_update_ns;
} TimingEngine;

// Timing parameters (in nanoseconds)
#define TPROP_LOOP1_MIN_NS 100
#define TPROP_LOOP1_MAX_NS 190
#define TPROP_LOOP2_MIN_NS 110
#define TPROP_LOOP2_MAX_NS 190
#define TPWRUP_US 340
#define TMODE1_US 200
#define TBIAS_US 200

// Initialize timing engine
void timing_engine_init(void);

// Advance simulation time
void timing_engine_advance(uint64_t delta_ns);

// Get current time
uint64_t timing_engine_get_time(void);

// Add propagation delay
uint64_t timing_engine_add_delay(uint64_t base_time, uint64_t delay_ns);

// Check if timeout has elapsed
bool timing_engine_is_timeout(uint64_t start_time, uint64_t timeout_ns);
```

### 9. Main Simulator

**責務**: 全コンポーネントを統合し、APIを提供

**インターフェース**:
```c
typedef struct {
    Pin pins[14];
    ModeState mode_state;
    CANTransceiver can_transceiver;
    PowerState power_state;
    FaultState fault_state;
    WakeState wake_state;
    BusBiasController bus_bias;
    TimingEngine timing;
    
    // Configuration
    double tj_temperature;
    double rl_resistance;
    double cl_capacitance;
} TCAN1463Q1Simulator;

// Initialize simulator
TCAN1463Q1Simulator* tcan1463q1_simulator_create(void);

// Destroy simulator
void tcan1463q1_simulator_destroy(TCAN1463Q1Simulator* sim);

// Reset simulator to initial state
void tcan1463q1_simulator_reset(TCAN1463Q1Simulator* sim);

// Unified I/O access interface
// Set pin value (for input pins and power pins)
bool tcan1463q1_simulator_set_pin(
    TCAN1463Q1Simulator* sim,
    PinType pin,
    PinState state,
    double voltage
);

// Get pin value (for output pins and bidirectional pins)
bool tcan1463q1_simulator_get_pin(
    TCAN1463Q1Simulator* sim,
    PinType pin,
    PinState* state,
    double* voltage
);

// Batch I/O operations for efficiency
typedef struct {
    PinType pin;
    PinState state;
    double voltage;
} PinValue;

// Set multiple pins at once
bool tcan1463q1_simulator_set_pins(
    TCAN1463Q1Simulator* sim,
    const PinValue* values,
    size_t count
);

// Get multiple pins at once
bool tcan1463q1_simulator_get_pins(
    TCAN1463Q1Simulator* sim,
    PinValue* values,
    size_t count
);

// Get pin metadata (direction, voltage range, etc.)
bool tcan1463q1_simulator_get_pin_info(
    TCAN1463Q1Simulator* sim,
    PinType pin,
    bool* is_input,
    bool* is_output,
    double* min_voltage,
    double* max_voltage
);

// Advance simulation time
void tcan1463q1_simulator_step(TCAN1463Q1Simulator* sim, uint64_t delta_ns);

// Run simulation until a condition is met or timeout
typedef bool (*SimulationCondition)(TCAN1463Q1Simulator* sim, void* user_data);

bool tcan1463q1_simulator_run_until(
    TCAN1463Q1Simulator* sim,
    SimulationCondition condition,
    void* user_data,
    uint64_t timeout_ns
);

// Get current operating mode
OperatingMode tcan1463q1_simulator_get_mode(TCAN1463Q1Simulator* sim);

// Get status flags
void tcan1463q1_simulator_get_flags(
    TCAN1463Q1Simulator* sim,
    bool* pwron,
    bool* wakerq,
    bool* wakesr,
    bool* uvsup,
    bool* uvcc,
    bool* uvio,
    bool* cbf,
    bool* txdclp,
    bool* txddto,
    bool* txdrxd,
    bool* candom,
    bool* tsd
);

// Configure parameters
void tcan1463q1_simulator_configure(
    TCAN1463Q1Simulator* sim,
    double vsup,
    double vcc,
    double vio,
    double tj_temperature,
    double rl_resistance,
    double cl_capacitance
);

// Snapshot and restore for testing
typedef struct {
    uint8_t* data;
    size_t size;
} SimulatorSnapshot;

// Create a snapshot of current simulator state
SimulatorSnapshot* tcan1463q1_simulator_snapshot(TCAN1463Q1Simulator* sim);

// Restore simulator state from snapshot
bool tcan1463q1_simulator_restore(
    TCAN1463Q1Simulator* sim,
    const SimulatorSnapshot* snapshot
);

// Free snapshot
void tcan1463q1_simulator_snapshot_free(SimulatorSnapshot* snapshot);

// Event callback system for monitoring
typedef enum {
    EVENT_MODE_CHANGE,
    EVENT_FAULT_DETECTED,
    EVENT_WAKE_UP,
    EVENT_PIN_CHANGE,
    EVENT_FLAG_CHANGE
} SimulatorEventType;

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

typedef void (*EventCallback)(const SimulatorEvent* event, void* user_data);

// Register event callback
bool tcan1463q1_simulator_register_callback(
    TCAN1463Q1Simulator* sim,
    SimulatorEventType event_type,
    EventCallback callback,
    void* user_data
);

// Unregister event callback
bool tcan1463q1_simulator_unregister_callback(
    TCAN1463Q1Simulator* sim,
    SimulatorEventType event_type,
    EventCallback callback
);
```

### 10. C Language Interface

**責務**: C言語からシミュレーターを使用可能にする

**インターフェース**:

```c
// C API Header (tcan1463q1_c_api.h)
#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle for C API
typedef struct TCAN1463Q1SimulatorHandle* TCAN1463Q1SimHandle;

// Error codes for C API
typedef enum {
    TCAN_SUCCESS = 0,
    TCAN_ERROR_INVALID_HANDLE = -1,
    TCAN_ERROR_INVALID_PIN = -2,
    TCAN_ERROR_INVALID_VOLTAGE = -3,
    TCAN_ERROR_INVALID_MODE = -4,
    TCAN_ERROR_INVALID_PARAMETER = -5,
    TCAN_ERROR_OUT_OF_MEMORY = -6,
    TCAN_ERROR_NULL_POINTER = -7
} TCAN_ErrorCode;

// Create simulator instance
TCAN_ErrorCode tcan_simulator_create(TCAN1463Q1SimHandle* handle);

// Destroy simulator instance
TCAN_ErrorCode tcan_simulator_destroy(TCAN1463Q1SimHandle handle);

// Reset simulator
TCAN_ErrorCode tcan_simulator_reset(TCAN1463Q1SimHandle handle);

// Set pin value
TCAN_ErrorCode tcan_simulator_set_pin(
    TCAN1463Q1SimHandle handle,
    int pin_type,
    int pin_state,
    double voltage
);

// Get pin value
TCAN_ErrorCode tcan_simulator_get_pin(
    TCAN1463Q1SimHandle handle,
    int pin_type,
    int* pin_state,
    double* voltage
);

// Advance simulation time
TCAN_ErrorCode tcan_simulator_step(
    TCAN1463Q1SimHandle handle,
    uint64_t delta_ns
);

// Get current operating mode
TCAN_ErrorCode tcan_simulator_get_mode(
    TCAN1463Q1SimHandle handle,
    int* mode
);

// Get status flags
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

// Configure parameters
TCAN_ErrorCode tcan_simulator_configure(
    TCAN1463Q1SimHandle handle,
    double vsup,
    double vcc,
    double vio,
    double tj_temperature,
    double rl_resistance,
    double cl_capacitance
);

// Get last error message
const char* tcan_get_error_string(TCAN_ErrorCode error_code);

#ifdef __cplusplus
}
#endif
```

**実装方針**:
- C++ APIをラップする薄いC互換レイヤーを提供
- 例外をエラーコードに変換
- オブジェクトポインタを不透明ハンドルとして扱う
- メモリ管理はC APIが責任を持つ

## Data Models

### Pin State Model

```c
typedef struct {
    PinType type;
    PinState state;
    double voltage;
    bool is_input;
    bool is_output;
    double min_voltage;
    double max_voltage;
} PinModel;
```

### Mode Transition Table

| From Mode     | To Mode       | Condition                                      |
|---------------|---------------|------------------------------------------------|
| Off           | Normal        | VSUP > UVSUP(R), EN=H, nSTB=H, VCC/VIO valid   |
| Off           | Silent        | VSUP > UVSUP(R), EN=L, nSTB=H, VCC/VIO valid   |
| Normal        | Silent        | EN: H→L                                        |
| Silent        | Normal        | EN: L→H                                        |
| Normal        | Standby       | nSTB: H→L, WAKERQ set                          |
| Silent        | Standby       | nSTB: H→L, WAKERQ set                          |
| Normal        | Go-to-sleep   | nSTB: H→L, WAKERQ cleared                      |
| Silent        | Go-to-sleep   | nSTB: H→L, WAKERQ cleared                      |
| Go-to-sleep   | Sleep         | t >= tSILENCE                                  |
| Standby       | Normal        | nSTB: L→H, EN=H                                |
| Standby       | Silent        | nSTB: L→H, EN=L                                |
| Sleep         | Standby       | WUP or LWU event                               |
| Any           | Off           | VSUP < UVSUP(F)                                |

### CAN Transceiver State Machine

| From State            | To State              | Condition                                      |
|-----------------------|-----------------------|------------------------------------------------|
| Off                   | Autonomous Inactive   | VSUP > UVSUP(R)                                |
| Autonomous Inactive   | Autonomous Active     | Remote wake-up (WUP)                           |
| Autonomous Active     | Autonomous Inactive   | t > tSILENCE                                   |
| Autonomous Inactive   | Active                | Enter Normal/Silent mode                       |
| Autonomous Active     | Active                | Enter Normal/Silent mode                       |
| Active                | Autonomous Inactive   | Exit to Standby/Go-to-sleep/Sleep, bus idle    |
| Active                | Autonomous Active     | Exit to Standby/Go-to-sleep/Sleep, bus active  |
| Any                   | Off                   | VSUP < UVSUP(F)                                |

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: Pin voltage validation

*For any* pin and voltage value, when setting a pin value, if the voltage is outside the pin's valid range, the operation should be rejected and return false.

**Validates: Requirements 1.5**

### Property 2: Pin value round-trip consistency

*For any* input pin and valid voltage value, after setting the pin value via the API, reading the pin value should return the same state and voltage.

**Validates: Requirements 1.6, 11.2**

### Property 3: Mode transition validity

*For any* current mode and target mode, when a mode transition is requested, the transition should only succeed if there exists a valid transition path in the mode transition table.

**Validates: Requirements 2.7**

### Property 4: CAN transceiver behavior consistency

*For any* operating mode and bus differential voltage, the CAN transceiver should behave according to the following rules: (1) In Silent mode, driver is disabled and receiver is enabled; (2) When Vdiff >= 0.9V, RXD is low; (3) When Vdiff <= 0.5V, RXD is high; (4) In Normal mode with TXD low, CANH is high and CANL is low.

**Validates: Requirements 3.1, 3.2, 3.3, 3.4, 3.5**

### Property 5: CAN FD data rate support

*For any* data rate up to 8 Mbps, the CAN transceiver should correctly transmit and receive data without timing violations.

**Validates: Requirements 3.6**

### Property 6: Undervoltage detection and flag management

*For any* supply voltage (VSUP, VCC, VIO), when the voltage drops below the falling threshold and remains there for t >= tUV, the corresponding UV flag should be set; when the voltage rises above the rising threshold, the flag should be cleared.

**Validates: Requirements 4.1, 4.2, 4.3, 4.5, 4.6**

### Property 7: PWRON flag on power-up

*For any* initial state, when VSUP transitions from below UVSUP(F) to above UVSUP(R), the PWRON flag should be set.

**Validates: Requirements 4.4**

### Property 8: Protected state on undervoltage

*For any* operating state, when UVCC or UVIO flag is set, the simulator should enter protected state with no bus load (bus pins in high impedance).

**Validates: Requirements 4.7**

### Property 9: TXD dominant timeout detection

*For any* TXD state, when TXD is held dominant (low) for t >= tTXDDTO (1.2-3.8ms), the TXDDTO flag should be set and the CAN driver should be disabled.

**Validates: Requirements 5.2**

### Property 10: TXD/RXD short detection

*For any* TXD and RXD state, when TXD and RXD are shorted (both same value) for t >= tTXDDTO, the TXDRXD flag should be set and the CAN driver should be disabled.

**Validates: Requirements 5.3**

### Property 11: CAN bus dominant fault detection

*For any* bus state, when the CAN bus is dominant for t >= tBUSDOM (1.4-3.8ms), the CANDOM flag should be set.

**Validates: Requirements 5.4**

### Property 12: Thermal shutdown detection

*For any* junction temperature, when TJ >= TSDR (165°C), the TSD flag should be set and the CAN driver should be disabled.

**Validates: Requirements 5.5**

### Property 13: Fault indication on nFAULT

*For any* fault flag (TXDCLP, TXDDTO, TXDRXD, CANDOM, TSD, CBF), when any fault flag is set, nFAULT pin should be driven low.

**Validates: Requirements 5.7**

### Property 14: Wake-up pattern recognition

*For any* bus state sequence in Standby or Sleep mode, when a valid WUP pattern (filtered dominant >= tWK_FILTER, filtered recessive >= tWK_FILTER, filtered dominant >= tWK_FILTER) is received within tWK_TIMEOUT, the WAKERQ flag should be set and the mode should transition to Standby.

**Validates: Requirements 6.1, 6.5**

### Property 15: Local wake-up detection

*For any* WAKE pin state in Sleep mode, when a transition (rising or falling edge) occurs on the WAKE pin, the WAKERQ flag should be set and the mode should transition to Standby.

**Validates: Requirements 6.2**

### Property 16: Wake-up flag indication

*For any* state, when WAKERQ flag is set, RXD and nFAULT pins should both be driven low.

**Validates: Requirements 6.3**

### Property 17: Wake-up flag clearing

*For any* state with WAKERQ flag set, when transitioning to Normal mode, the WAKERQ flag should be cleared.

**Validates: Requirements 6.4**

### Property 18: INH pin control

*For any* operating mode, the INH pin should be: (1) driven high (VSUP - 0.5V to 1V) when in Normal, Silent, or Standby mode; (2) set to high impedance when in Sleep mode; (3) controlled by INH_MASK pin (disabled when INH_MASK is high, enabled when low or floating).

**Validates: Requirements 7.1, 7.2, 7.3, 7.4**

### Property 19: Bus bias control

*For any* CAN transceiver state, the bus bias should be: (1) VCC/2 in CAN active mode; (2) 2.5V in CAN autonomous active mode; (3) GND in CAN autonomous inactive mode; (4) high impedance in CAN off mode.

**Validates: Requirements 8.1, 8.2, 8.3, 8.4**

### Property 20: Autonomous state transition on silence

*For any* bus activity state in CAN autonomous active mode, when the bus is inactive for t > tSILENCE (0.6-1.2s), the transceiver should transition to CAN autonomous inactive mode.

**Validates: Requirements 8.5**

### Property 21: Autonomous state transition on wake-up

*For any* state in CAN autonomous inactive mode, when a remote wake-up occurs, the transceiver should transition to CAN autonomous active mode.

**Validates: Requirements 8.6**

### Property 22: Propagation delay bounds

*For any* TXD transition, the propagation delay from TXD to RXD should be within the specified bounds (100-190ns for recessive-to-dominant, 110-190ns for dominant-to-recessive).

**Validates: Requirements 9.1, 9.2**

### Property 23: Flag clearing on mode transition to Normal

*For any* status flags, when transitioning to Normal mode, the PWRON and WAKERQ flags should be cleared.

**Validates: Requirements 10.9**

### Property 24: Simulation time monotonicity

*For any* simulation step, when advancing simulation time, the new time should always be greater than or equal to the previous time.

**Validates: Requirements 11.3**

### Property 25: Parameter validation

*For any* configuration parameter, when setting a parameter value, if the value is outside the valid range specified in the datasheet, the operation should be rejected.

**Validates: Requirements 12.6**

## Error Handling

### Input Validation

- **Pin voltage out of range**: Return error code, do not update pin state
- **Invalid mode transition**: Return error code, maintain current mode
- **Invalid parameter value**: Return error code, maintain current configuration

### Fault Conditions

- **TXD clamped low**: Set TXDCLP flag, disable CAN driver, drive nFAULT low
- **TXD dominant timeout**: Set TXDDTO flag, disable CAN driver, drive nFAULT low
- **TXD/RXD short**: Set TXDRXD flag, disable CAN driver, drive nFAULT low
- **CAN bus dominant fault**: Set CANDOM flag, drive nFAULT low, keep driver enabled
- **Thermal shutdown**: Set TSD flag, disable CAN driver, drive nFAULT low
- **CAN bus fault**: Set CBF flag, drive nFAULT low, keep driver enabled
- **Undervoltage**: Set UV flags, enter protected state

### Recovery

- **Fault clearing**: Faults are cleared based on specific conditions (e.g., TXD transition, mode change)
- **Undervoltage recovery**: UV flags cleared when voltage returns above rising threshold
- **Mode recovery**: System can transition to normal operation after fault conditions are resolved

## Testing Strategy

### Unit Testing

単体テストでは、各コンポーネントの個別機能を検証します：

- **Pin Manager**: ピン値の設定/取得、電圧範囲検証
- **Mode Controller**: モード遷移ロジック、遷移条件の検証
- **CAN Transceiver**: バス状態検出、ドライバ/レシーバ制御
- **Power Monitor**: アンダーボルテージ検出、タイミング検証
- **Fault Detector**: 各フォールト条件の検出、フラグ設定
- **Wake Handler**: WUPパターン認識、LWU検出
- **Bus Bias Controller**: バイアス状態遷移、電圧設定
- **Timing Engine**: 時間進行、タイムアウト判定

### Property-Based Testing

プロパティベーステストでは、ランダムな入力に対してシステムの不変条件を検証します。C/C++では、以下のライブラリを使用します：

- **RapidCheck**: C++用のプロパティベーステストライブラリ
- **テスト設定**: 各プロパティテストは最低100回の反復を実行

各プロパティテストは、設計書のプロパティ番号を参照するタグを含めます：

```cpp
// Feature: tcan1463-q1-simulator, Property 1: Pin voltage validation
RC_GTEST_PROP(PinManagerTest, PinVoltageValidation, ()) {
    // Test implementation
}
```

### Integration Testing

統合テストでは、複数のコンポーネント間の相互作用を検証します：

- **モード遷移シナリオ**: Normal → Standby → Sleep → Normal
- **ウェイクアップシナリオ**: Sleep → WUP受信 → Standby → Normal
- **フォールト検出シナリオ**: Normal → TXD timeout → Fault → Recovery
- **電源シーケンス**: Power-up → Normal operation → Undervoltage → Recovery

### End-to-End Testing

エンドツーエンドテストでは、実際の使用シナリオを検証します：

- **CAN通信シミュレーション**: メッセージ送受信、バス状態遷移
- **低電力モード遷移**: Normal → Sleep → Wake-up → Normal
- **フォールトリカバリー**: 各種フォールト発生 → 検出 → リカバリー
- **長時間動作**: タイムアウト、タイマー動作の検証
