# Requirements Document

## Introduction

TCAN1463-Q1 CANトランシーバーICのソフトウェアシミュレーターを開発します。このシミュレーターは、実際のハードウェアICの動作を忠実に再現し、CAN/CAN FD通信、複数の動作モード、ピン入出力、電源管理、フォールト検出、ウェイクアップ機能などをシミュレートします。開発者がハードウェアなしでシステムの動作を検証できるようにすることが目的です。

## Glossary

- **Simulator**: TCAN1463-Q1 ICの動作をソフトウェアで再現するシステム
- **CAN_Transceiver**: CANバス通信を行うトランシーバー機能
- **Pin_Manager**: ICのピン入出力を管理するコンポーネント
- **Mode_Controller**: 動作モード（Normal, Silent, Standby, Go-to-sleep, Sleep, Off）を制御するコンポーネント
- **Power_Monitor**: 電源電圧（VSUP, VCC, VIO）を監視するコンポーネント
- **Fault_Detector**: バス故障、TXDタイムアウト、熱シャットダウンなどのフォールトを検出するコンポーネント
- **Wake_Handler**: リモート/ローカルウェイクアップイベントを処理するコンポーネント
- **Bus_Bias_Controller**: CANバスのバイアス状態を制御するコンポーネント

## Requirements

### Requirement 1: ピン入出力管理

**User Story:** As a developer, I want to simulate all IC pins, so that I can test pin-level interactions.

#### Acceptance Criteria

1. THE Simulator SHALL provide digital input pins (TXD, EN, nSTB, WAKE, INH_MASK)
2. THE Simulator SHALL provide digital output pins (RXD, nFAULT, INH)
3. THE Simulator SHALL provide bidirectional analog pins (CANH, CANL)
4. THE Simulator SHALL provide power supply pins (VSUP, VCC, VIO, GND)
5. WHEN a pin value is set, THE Pin_Manager SHALL validate the value against the pin's voltage range
6. WHEN a pin value is read, THE Pin_Manager SHALL return the current pin state

### Requirement 2: 動作モード制御

**User Story:** As a developer, I want to simulate all operating modes, so that I can verify mode transitions.

#### Acceptance Criteria

1. THE Simulator SHALL support Normal mode (EN=high, nSTB=high)
2. THE Simulator SHALL support Silent mode (EN=low, nSTB=high)
3. THE Simulator SHALL support Standby mode (EN=high or low, nSTB=low, WAKERQ set)
4. THE Simulator SHALL support Go-to-sleep mode (EN=high, nSTB=low, WAKERQ cleared, transitional)
5. THE Simulator SHALL support Sleep mode (EN=high, nSTB=low, after tSILENCE timeout)
6. THE Simulator SHALL support Off mode (VSUP < UVSUP)
7. WHEN mode control pins change, THE Mode_Controller SHALL transition to the appropriate mode
8. WHEN in Sleep mode for tSILENCE duration, THE Mode_Controller SHALL transition from Go-to-sleep to Sleep

### Requirement 3: CAN通信機能

**User Story:** As a developer, I want to simulate CAN/CAN FD communication, so that I can test bus interactions.

#### Acceptance Criteria

1. WHEN in Normal mode with TXD=low, THE CAN_Transceiver SHALL drive CANH high and CANL low (dominant)
2. WHEN in Normal mode with TXD=high, THE CAN_Transceiver SHALL set CANH and CANL to high impedance (recessive)
3. WHEN CANH-CANL differential voltage >= 0.9V, THE CAN_Transceiver SHALL set RXD=low (dominant detected)
4. WHEN CANH-CANL differential voltage <= 0.5V, THE CAN_Transceiver SHALL set RXD=high (recessive detected)
5. WHEN in Silent mode, THE CAN_Transceiver SHALL disable the driver but keep the receiver active
6. THE CAN_Transceiver SHALL support data rates up to 8 Mbps for CAN FD

### Requirement 4: 電源管理

**User Story:** As a developer, I want to simulate power supply monitoring, so that I can test undervoltage conditions.

#### Acceptance Criteria

1. WHEN VSUP < UVSUP(F) (3.5V-4.25V), THE Power_Monitor SHALL set UVSUP flag and enter Off mode
2. WHEN VCC < UVCC(F) (3.5V-3.9V) for t >= tUV (100-350ms), THE Power_Monitor SHALL set UVCC flag
3. WHEN VIO < UVIO(F) (1.0V-1.25V) for t >= tUV, THE Power_Monitor SHALL set UVIO flag
4. WHEN VSUP > UVSUP(R) (3.85V-4.4V), THE Power_Monitor SHALL clear UVSUP flag and set PWRON flag
5. WHEN VCC > UVCC(R) (4.1V-4.4V), THE Power_Monitor SHALL clear UVCC flag
6. WHEN VIO > UVIO(R) (1.4V-1.65V), THE Power_Monitor SHALL clear UVIO flag
7. WHEN UVCC or UVIO flag is set, THE Simulator SHALL enter protected state with no bus load

### Requirement 5: フォールト検出

**User Story:** As a developer, I want to simulate fault detection, so that I can test error handling.

#### Acceptance Criteria

1. WHEN TXD is low before entering Normal mode, THE Fault_Detector SHALL set TXDCLP flag and disable CAN driver
2. WHEN TXD is dominant for t >= tTXDDTO (1.2-3.8ms), THE Fault_Detector SHALL set TXDDTO flag and disable CAN driver
3. WHEN TXD and RXD are shorted for t >= tTXDDTO, THE Fault_Detector SHALL set TXDRXD flag and disable CAN driver
4. WHEN CAN bus is dominant for t >= tBUSDOM (1.4-3.8ms), THE Fault_Detector SHALL set CANDOM flag
5. WHEN junction temperature TJ >= TSDR (165°C), THE Fault_Detector SHALL set TSD flag and disable CAN driver
6. WHEN CAN bus fault persists for four consecutive dominant-to-recessive transitions, THE Fault_Detector SHALL set CBF flag
7. WHEN a fault is detected, THE Fault_Detector SHALL drive nFAULT pin low

### Requirement 6: ウェイクアップ機能

**User Story:** As a developer, I want to simulate wake-up events, so that I can test low-power mode recovery.

#### Acceptance Criteria

1. WHEN in Standby or Sleep mode and a valid WUP is received on CAN bus, THE Wake_Handler SHALL set WAKERQ flag and transition to Standby mode
2. WHEN WAKE pin transitions (rising or falling edge) in Sleep mode, THE Wake_Handler SHALL set WAKERQ flag and transition to Standby mode
3. WHEN WAKERQ flag is set, THE Wake_Handler SHALL drive RXD low and nFAULT low
4. WHEN transitioning to Normal mode, THE Wake_Handler SHALL clear WAKERQ flag
5. THE Wake_Handler SHALL recognize WUP pattern: filtered dominant, filtered recessive, filtered dominant (each >= tWK_FILTER 0.5-1.8μs)
6. WHEN WUP timeout tWK_TIMEOUT (0.8-2ms) expires without valid pattern, THE Wake_Handler SHALL reset WUP detection

### Requirement 7: INH制御

**User Story:** As a developer, I want to simulate INH pin behavior, so that I can test system power control.

#### Acceptance Criteria

1. WHEN in Normal, Silent, or Standby mode, THE Simulator SHALL drive INH pin high (VSUP - 0.5V to 1V)
2. WHEN in Sleep mode, THE Simulator SHALL set INH pin to high impedance
3. WHEN INH_MASK pin is high, THE Simulator SHALL disable INH functionality
4. WHEN INH_MASK pin is low or floating, THE Simulator SHALL enable INH functionality
5. WHEN wake-up event occurs, THE Simulator SHALL assert INH after tINH_SLP_STB (100μs)

### Requirement 8: バスバイアス制御

**User Story:** As a developer, I want to simulate autonomous bus biasing, so that I can test CAN transceiver states.

#### Acceptance Criteria

1. WHEN in CAN active mode, THE Bus_Bias_Controller SHALL bias bus to VCC/2
2. WHEN in CAN autonomous active mode, THE Bus_Bias_Controller SHALL bias bus to 2.5V
3. WHEN in CAN autonomous inactive mode, THE Bus_Bias_Controller SHALL bias bus to GND
4. WHEN in CAN off mode, THE Bus_Bias_Controller SHALL set bus pins to high impedance
5. WHEN bus is inactive for t > tSILENCE (0.6-1.2s), THE Bus_Bias_Controller SHALL transition from autonomous active to autonomous inactive
6. WHEN remote wake-up occurs, THE Bus_Bias_Controller SHALL transition from autonomous inactive to autonomous active

### Requirement 9: タイミング制御

**User Story:** As a developer, I want to simulate timing characteristics, so that I can verify timing-dependent behavior.

#### Acceptance Criteria

1. THE Simulator SHALL simulate propagation delay tPROP(LOOP1) (100-190ns) from TXD to RXD for recessive-to-dominant
2. THE Simulator SHALL simulate propagation delay tPROP(LOOP2) (110-190ns) from TXD to RXD for dominant-to-recessive
3. THE Simulator SHALL simulate power-up time tPWRUP (340μs) for INH activation after VSUP >= UVSUP
4. THE Simulator SHALL simulate mode change time tMODE1 (200μs) from Sleep to Normal/Silent mode
5. THE Simulator SHALL simulate bus bias reaction time tBIAS (200μs)
6. THE Simulator SHALL simulate bus fault detection time tCBF (2.5μs)

### Requirement 10: 状態フラグ管理

**User Story:** As a developer, I want to simulate all status flags, so that I can test flag-based logic.

#### Acceptance Criteria

1. THE Simulator SHALL maintain PWRON flag (power-up indicator)
2. THE Simulator SHALL maintain WAKERQ flag (wake-up request)
3. THE Simulator SHALL maintain WAKESR flag (wake-up source recognition)
4. THE Simulator SHALL maintain INHMASK flag (INH_MASK change indicator)
5. THE Simulator SHALL maintain UVCC, UVIO, UVSUP flags (undervoltage indicators)
6. THE Simulator SHALL maintain CBF flag (CAN bus fault)
7. THE Simulator SHALL maintain TXDCLP, TXDDTO, TXDRXD, CANDOM, TSD flags (local faults)
8. WHEN entering Silent mode from Standby/Sleep with PWRON set, THE Simulator SHALL drive nFAULT low
9. WHEN entering Normal mode, THE Simulator SHALL clear PWRON and WAKERQ flags

### Requirement 11: シミュレーション制御API

**User Story:** As a developer, I want to control the simulator programmatically, so that I can automate testing.

#### Acceptance Criteria

1. THE Simulator SHALL provide an API to set pin values
2. THE Simulator SHALL provide an API to read pin values
3. THE Simulator SHALL provide an API to advance simulation time
4. THE Simulator SHALL provide an API to query current operating mode
5. THE Simulator SHALL provide an API to query status flags
6. THE Simulator SHALL provide an API to reset the simulator to initial state

### Requirement 12: 設定とパラメータ

**User Story:** As a developer, I want to configure simulator parameters, so that I can test different operating conditions.

#### Acceptance Criteria

1. THE Simulator SHALL allow configuration of supply voltages (VSUP, VCC, VIO)
2. THE Simulator SHALL allow configuration of junction temperature (TJ)
3. THE Simulator SHALL allow configuration of bus load resistance (RL)
4. THE Simulator SHALL allow configuration of bus capacitance (CL)
5. THE Simulator SHALL allow configuration of timing parameters (tTXDDTO, tBUSDOM, tSILENCE, etc.)
6. WHEN parameters are configured, THE Simulator SHALL validate them against specified ranges

### Requirement 13: C言語インターフェース

**User Story:** As a C developer, I want to use the simulator from C code, so that I can integrate it with existing C projects.

#### Acceptance Criteria

1. THE Simulator SHALL provide a C-compatible API with extern "C" linkage
2. THE Simulator SHALL use only C-compatible types in the public C API (no C++ classes, templates, or exceptions in C headers)
3. THE Simulator SHALL provide opaque handle types for C API users
4. THE Simulator SHALL provide C-compatible error codes instead of exceptions for error handling
5. THE Simulator SHALL provide header files that can be included in both C and C++ projects
6. THE Simulator SHALL document all C API functions with usage examples
7. THE Simulator SHALL ensure binary compatibility across C and C++ compilation units
