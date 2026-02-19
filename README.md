# TCAN1463-Q1 Simulator

A software simulator for the TCAN1463-Q1 CAN transceiver IC, implemented in C/C++.

## Project Structure

```
.
├── include/                    # Public header files
│   ├── tcan1463q1_types.h     # Core data types and enumerations
│   ├── tcan1463q1_simulator.h # Main simulator API
│   └── tcan1463q1_scenario.h  # Scenario framework API
├── src/                        # Implementation files
│   ├── pin_manager.cpp
│   ├── mode_controller.cpp
│   ├── can_transceiver.cpp
│   ├── power_monitor.cpp
│   ├── fault_detector.cpp
│   ├── wake_handler.cpp
│   ├── bus_bias_controller.cpp
│   ├── timing_engine.cpp
│   ├── simulator.cpp
│   ├── scenario.cpp
│   └── c_api.cpp
├── test/                       # Test files
│   └── test_main.cpp
├── examples/                   # Example programs
│   └── scenario_example.cpp
├── CMakeLists.txt             # Build configuration
└── README.md                  # This file
```

## Building

```bash
mkdir build
cd build
cmake ..
make -j14  # Use all 14 cores for parallel build
```

## Running Tests

```bash
cd build
ctest
# or
./tcan1463q1_tests
```

## Running Examples

```bash
cd build
./scenario_example
```

## Features

- Pin I/O management for all TCAN1463-Q1 pins
- Operating mode control (Normal, Silent, Standby, Go-to-sleep, Sleep, Off)
- CAN/CAN FD communication simulation
- Power supply monitoring and undervoltage detection
- Fault detection (TXD timeout, bus faults, thermal shutdown, etc.)
- Wake-up event handling (remote and local)
- Bus bias control
- Nanosecond-precision timing simulation
- **Event callback system** - Register callbacks for mode changes, faults, wake-ups, pin changes, and flag changes
- **Scenario-based testing framework** - Define and execute test scenarios
- Pre-defined scenarios for common use cases
- C and C++ API support
- Property-based testing with RapidCheck

## Scenario Framework

The simulator includes a powerful scenario framework for defining and executing test sequences:

```cpp
// Create a custom scenario
Scenario* scenario = tcan1463q1_scenario_create("My Test", "Description");

// Add actions
tcan1463q1_scenario_add_configure(scenario, "Setup", 5.0, 5.0, 3.3, 25.0, 60.0, 100e-12);
tcan1463q1_scenario_add_set_pin(scenario, "EN high", PIN_EN, PIN_STATE_HIGH, 3.3);
tcan1463q1_scenario_add_wait(scenario, "Wait 1ms", 1000000);
tcan1463q1_scenario_add_check_mode(scenario, "Verify Normal", MODE_NORMAL);

// Execute
ScenarioResult result = tcan1463q1_scenario_execute(scenario, sim);
tcan1463q1_scenario_result_print(&result);
```

Pre-defined scenarios:
- Power-up sequence
- Normal to Sleep transition
- Sleep to Normal via wake-up
- CAN message transmission
- Fault detection scenarios
- Undervoltage recovery

See `examples/scenario_example.cpp` for complete examples.

## Event Callback System

The simulator supports event callbacks for monitoring state changes:

```cpp
// Define a callback function
void on_mode_change(const SimulatorEvent* event, void* user_data) {
    printf("Mode changed from %d to %d\n",
           event->data.mode_change.old_mode,
           event->data.mode_change.new_mode);
}

// Register the callback
tcan1463q1_simulator_register_callback(sim, EVENT_MODE_CHANGE, on_mode_change, NULL);

// Unregister when done
tcan1463q1_simulator_unregister_callback(sim, EVENT_MODE_CHANGE, on_mode_change);
```

Supported event types:
- `EVENT_MODE_CHANGE` - Operating mode transitions
- `EVENT_FAULT_DETECTED` - Fault conditions detected
- `EVENT_WAKE_UP` - Wake-up events
- `EVENT_PIN_CHANGE` - Pin state changes
- `EVENT_FLAG_CHANGE` - Status flag changes

## Requirements

- CMake 3.14 or higher
- C++17 compatible compiler
- C11 compatible compiler
- Internet connection (for fetching dependencies)

## Dependencies

The following dependencies are automatically fetched by CMake:
- GoogleTest (for unit testing)
- RapidCheck (for property-based testing)

## License

TBD
