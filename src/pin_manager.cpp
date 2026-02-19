#include "pin_manager.h"
#include <string.h>

// Pin voltage ranges based on TCAN1463-Q1 datasheet
static const struct {
    double min;
    double max;
} pin_voltage_ranges[14] = {
    {0.0, 5.5},    // PIN_TXD - Digital input (VIO domain)
    {0.0, 5.5},    // PIN_RXD - Digital output (VIO domain)
    {0.0, 5.5},    // PIN_EN - Digital input (VIO domain)
    {0.0, 5.5},    // PIN_NSTB - Digital input (VIO domain)
    {0.0, 5.5},    // PIN_NFAULT - Digital output (VIO domain)
    {0.0, 5.5},    // PIN_WAKE - Digital input (VIO domain)
    {0.0, 42.0},   // PIN_INH - Output (VSUP domain, up to 42V)
    {0.0, 5.5},    // PIN_INH_MASK - Digital input (VIO domain)
    {-27.0, 42.0}, // PIN_CANH - Bidirectional analog (wide range)
    {-27.0, 42.0}, // PIN_CANL - Bidirectional analog (wide range)
    {4.5, 42.0},   // PIN_VSUP - Power supply (4.5V to 42V nominal)
    {4.5, 5.5},    // PIN_VCC - Logic supply (5V nominal)
    {1.65, 5.5},   // PIN_VIO - I/O supply (1.8V to 5V)
    {0.0, 0.0}     // PIN_GND - Ground (always 0V)
};

// Pin direction configuration
static const struct {
    bool is_input;
    bool is_output;
} pin_directions[14] = {
    {true, false},  // PIN_TXD - Input
    {false, true},  // PIN_RXD - Output
    {true, false},  // PIN_EN - Input
    {true, false},  // PIN_NSTB - Input
    {false, true},  // PIN_NFAULT - Output
    {true, false},  // PIN_WAKE - Input
    {false, true},  // PIN_INH - Output
    {true, false},  // PIN_INH_MASK - Input
    {true, true},   // PIN_CANH - Bidirectional
    {true, true},   // PIN_CANL - Bidirectional
    {true, false},  // PIN_VSUP - Power (input)
    {true, false},  // PIN_VCC - Power (input)
    {true, false},  // PIN_VIO - Power (input)
    {true, false}   // PIN_GND - Ground (input)
};

void pin_init(Pin* pin, PinType type, bool is_input, bool is_output,
              double min_voltage, double max_voltage) {
    if (!pin) return;
    
    pin->state = PIN_STATE_LOW;
    pin->voltage = 0.0;
    pin->is_input = is_input;
    pin->is_output = is_output;
    pin->min_voltage = min_voltage;
    pin->max_voltage = max_voltage;
}

bool pin_set_value(Pin* pin, PinState state, double voltage) {
    if (!pin) return false;
    
    // Validate voltage range
    if (!pin_validate_voltage(pin, voltage)) {
        return false;
    }
    
    pin->state = state;
    pin->voltage = voltage;
    return true;
}

bool pin_get_value(const Pin* pin, PinState* state, double* voltage) {
    if (!pin) return false;
    
    if (state) {
        *state = pin->state;
    }
    if (voltage) {
        *voltage = pin->voltage;
    }
    return true;
}

bool pin_validate_voltage(const Pin* pin, double voltage) {
    if (!pin) return false;
    
    // For digital states (LOW, HIGH, HIGH_IMPEDANCE), voltage validation is relaxed
    // Only strictly validate for ANALOG state or when voltage is explicitly set
    if (pin->state != PIN_STATE_ANALOG && voltage == 0.0) {
        return true;  // Default voltage for digital states
    }
    
    // Check if voltage is within valid range
    if (voltage < pin->min_voltage || voltage > pin->max_voltage) {
        return false;
    }
    
    return true;
}

void pin_manager_init(PinManager* manager) {
    if (!manager) return;
    
    // Initialize all pins with their properties
    for (int i = 0; i < 14; i++) {
        pin_init(&manager->pins[i], 
                (PinType)i,
                pin_directions[i].is_input,
                pin_directions[i].is_output,
                pin_voltage_ranges[i].min,
                pin_voltage_ranges[i].max);
    }
    
    // Set default states
    // Digital inputs default to LOW
    manager->pins[PIN_TXD].state = PIN_STATE_HIGH;  // TXD defaults to recessive (high)
    manager->pins[PIN_EN].state = PIN_STATE_LOW;
    manager->pins[PIN_NSTB].state = PIN_STATE_LOW;
    manager->pins[PIN_WAKE].state = PIN_STATE_LOW;
    manager->pins[PIN_INH_MASK].state = PIN_STATE_LOW;
    
    // Digital outputs default to their inactive states
    manager->pins[PIN_RXD].state = PIN_STATE_HIGH;  // RXD defaults to recessive (high)
    manager->pins[PIN_NFAULT].state = PIN_STATE_HIGH;  // nFAULT inactive (high)
    manager->pins[PIN_INH].state = PIN_STATE_HIGH_IMPEDANCE;  // INH starts in high-Z
    
    // Bidirectional pins default to high impedance
    manager->pins[PIN_CANH].state = PIN_STATE_HIGH_IMPEDANCE;
    manager->pins[PIN_CANH].voltage = 0.0;
    manager->pins[PIN_CANL].state = PIN_STATE_HIGH_IMPEDANCE;
    manager->pins[PIN_CANL].voltage = 0.0;
    
    // Power pins default to nominal voltages
    manager->pins[PIN_VSUP].state = PIN_STATE_ANALOG;
    manager->pins[PIN_VSUP].voltage = 12.0;  // Typical automotive voltage
    manager->pins[PIN_VCC].state = PIN_STATE_ANALOG;
    manager->pins[PIN_VCC].voltage = 5.0;
    manager->pins[PIN_VIO].state = PIN_STATE_ANALOG;
    manager->pins[PIN_VIO].voltage = 3.3;
    manager->pins[PIN_GND].state = PIN_STATE_ANALOG;
    manager->pins[PIN_GND].voltage = 0.0;
}

bool pin_manager_set_pin(PinManager* manager, PinType pin_type,
                         PinState state, double voltage) {
    if (!manager || pin_type < 0 || pin_type >= 14) {
        return false;
    }
    
    Pin* pin = &manager->pins[pin_type];
    
    // Check if pin is an input (can be set externally)
    if (!pin->is_input) {
        return false;  // Cannot set output-only pins
    }
    
    return pin_set_value(pin, state, voltage);
}

bool pin_manager_get_pin(const PinManager* manager, PinType pin_type,
                         PinState* state, double* voltage) {
    if (!manager || pin_type < 0 || pin_type >= 14) {
        return false;
    }
    
    const Pin* pin = &manager->pins[pin_type];
    return pin_get_value(pin, state, voltage);
}

bool pin_manager_validate_pin(const PinManager* manager, PinType pin_type,
                               double voltage) {
    if (!manager || pin_type < 0 || pin_type >= 14) {
        return false;
    }
    
    const Pin* pin = &manager->pins[pin_type];
    return pin_validate_voltage(pin, voltage);
}

bool pin_manager_get_pin_info(const PinManager* manager, PinType pin_type,
                               bool* is_input, bool* is_output,
                               double* min_voltage, double* max_voltage) {
    if (!manager || pin_type < 0 || pin_type >= 14) {
        return false;
    }
    
    const Pin* pin = &manager->pins[pin_type];
    
    if (is_input) {
        *is_input = pin->is_input;
    }
    if (is_output) {
        *is_output = pin->is_output;
    }
    if (min_voltage) {
        *min_voltage = pin->min_voltage;
    }
    if (max_voltage) {
        *max_voltage = pin->max_voltage;
    }
    
    return true;
}
