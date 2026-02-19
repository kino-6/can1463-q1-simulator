#ifndef TIMING_ENGINE_H
#define TIMING_ENGINE_H

#include "tcan1463q1_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize timing engine
 * Sets current time to 0
 * 
 * @param engine Pointer to timing engine structure
 */
void timing_engine_init(TimingEngine* engine);

/**
 * Advance simulation time by delta nanoseconds
 * 
 * @param engine Pointer to timing engine structure
 * @param delta_ns Time delta in nanoseconds
 */
void timing_engine_advance(TimingEngine* engine, uint64_t delta_ns);

/**
 * Get current simulation time in nanoseconds
 * 
 * @param engine Pointer to timing engine structure
 * @return Current time in nanoseconds
 */
uint64_t timing_engine_get_time(const TimingEngine* engine);

/**
 * Add propagation delay to a base time
 * 
 * @param engine Pointer to timing engine structure
 * @param base_time Base time in nanoseconds
 * @param delay_ns Delay to add in nanoseconds
 * @return Resulting time in nanoseconds
 */
uint64_t timing_engine_add_delay(const TimingEngine* engine, uint64_t base_time, uint64_t delay_ns);

/**
 * Check if a timeout has elapsed
 * 
 * @param engine Pointer to timing engine structure
 * @param start_time Start time in nanoseconds
 * @param timeout_ns Timeout duration in nanoseconds
 * @return true if timeout has elapsed, false otherwise
 */
bool timing_engine_is_timeout(const TimingEngine* engine, uint64_t start_time, uint64_t timeout_ns);

#ifdef __cplusplus
}
#endif

#endif // TIMING_ENGINE_H
