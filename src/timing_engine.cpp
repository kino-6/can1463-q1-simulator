#include "timing_engine.h"
#include <string.h>

void timing_engine_init(TimingEngine* engine) {
    if (engine == NULL) {
        return;
    }
    
    engine->current_time_ns = 0;
    engine->last_update_ns = 0;
}

void timing_engine_advance(TimingEngine* engine, uint64_t delta_ns) {
    if (engine == NULL) {
        return;
    }
    
    engine->last_update_ns = engine->current_time_ns;
    engine->current_time_ns += delta_ns;
}

uint64_t timing_engine_get_time(const TimingEngine* engine) {
    if (engine == NULL) {
        return 0;
    }
    
    return engine->current_time_ns;
}

uint64_t timing_engine_add_delay(const TimingEngine* engine, uint64_t base_time, uint64_t delay_ns) {
    // Engine parameter is for future extensibility (e.g., jitter simulation)
    (void)engine;
    
    return base_time + delay_ns;
}

bool timing_engine_is_timeout(const TimingEngine* engine, uint64_t start_time, uint64_t timeout_ns) {
    if (engine == NULL) {
        return false;
    }
    
    uint64_t current_time = engine->current_time_ns;
    uint64_t elapsed = current_time - start_time;
    
    return elapsed >= timeout_ns;
}
