#ifndef PIN_MANAGER_H
#define PIN_MANAGER_H

#include "tcan1463q1_types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Pin Manager - Manages all pin states and I/O operations
 */

/**
 * Initialize a pin with its properties
 * @param pin Pointer to pin structure
 * @param type Pin type
 * @param is_input Whether pin is an input
 * @param is_output Whether pin is an output
 * @param min_voltage Minimum valid voltage
 * @param max_voltage Maximum valid voltage
 */
void pin_init(Pin* pin, PinType type, bool is_input, bool is_output,
              double min_voltage, double max_voltage);

/**
 * Set pin value (for input pins)
 * @param pin Pointer to pin structure
 * @param state Pin state
 * @param voltage Pin voltage (for analog pins)
 * @return true if successful, false if validation fails
 */
bool pin_set_value(Pin* pin, PinState state, double voltage);

/**
 * Get pin value (for output pins)
 * @param pin Pointer to pin structure
 * @param state Pointer to store pin state
 * @param voltage Pointer to store pin voltage
 * @return true if successful
 */
bool pin_get_value(const Pin* pin, PinState* state, double* voltage);

/**
 * Validate pin voltage against range
 * @param pin Pointer to pin structure
 * @param voltage Voltage to validate
 * @return true if voltage is within valid range
 */
bool pin_validate_voltage(const Pin* pin, double voltage);

/**
 * Pin Manager structure
 */
typedef struct {
    Pin pins[14];  // All 14 pins of TCAN1463-Q1
} PinManager;

/**
 * Initialize pin manager with all pins
 * @param manager Pointer to pin manager structure
 */
void pin_manager_init(PinManager* manager);

/**
 * Set pin value by pin type
 * @param manager Pointer to pin manager
 * @param pin_type Type of pin to set
 * @param state Pin state
 * @param voltage Pin voltage
 * @return true if successful, false if validation fails
 */
bool pin_manager_set_pin(PinManager* manager, PinType pin_type,
                         PinState state, double voltage);

/**
 * Get pin value by pin type
 * @param manager Pointer to pin manager
 * @param pin_type Type of pin to get
 * @param state Pointer to store pin state
 * @param voltage Pointer to store pin voltage
 * @return true if successful
 */
bool pin_manager_get_pin(const PinManager* manager, PinType pin_type,
                         PinState* state, double* voltage);

/**
 * Validate pin voltage by pin type
 * @param manager Pointer to pin manager
 * @param pin_type Type of pin to validate
 * @param voltage Voltage to validate
 * @return true if voltage is within valid range
 */
bool pin_manager_validate_pin(const PinManager* manager, PinType pin_type,
                               double voltage);

/**
 * Get pin metadata
 * @param manager Pointer to pin manager
 * @param pin_type Type of pin
 * @param is_input Pointer to store input flag
 * @param is_output Pointer to store output flag
 * @param min_voltage Pointer to store minimum voltage
 * @param max_voltage Pointer to store maximum voltage
 * @return true if successful
 */
bool pin_manager_get_pin_info(const PinManager* manager, PinType pin_type,
                               bool* is_input, bool* is_output,
                               double* min_voltage, double* max_voltage);

#ifdef __cplusplus
}
#endif

#endif // PIN_MANAGER_H
