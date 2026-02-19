#include <gtest/gtest.h>
#include <rapidcheck.h>
#include "timing_engine.h"

// Unit tests for Timing Engine

TEST(TimingEngineTest, InitializesToZero) {
    TimingEngine engine;
    timing_engine_init(&engine);
    
    EXPECT_EQ(timing_engine_get_time(&engine), 0);
}

TEST(TimingEngineTest, AdvanceTime) {
    TimingEngine engine;
    timing_engine_init(&engine);
    
    timing_engine_advance(&engine, 1000);
    EXPECT_EQ(timing_engine_get_time(&engine), 1000);
    
    timing_engine_advance(&engine, 500);
    EXPECT_EQ(timing_engine_get_time(&engine), 1500);
}

TEST(TimingEngineTest, AddDelay) {
    TimingEngine engine;
    timing_engine_init(&engine);
    
    uint64_t base_time = 1000;
    uint64_t delay = 500;
    uint64_t result = timing_engine_add_delay(&engine, base_time, delay);
    
    EXPECT_EQ(result, 1500);
}

TEST(TimingEngineTest, TimeoutNotElapsed) {
    TimingEngine engine;
    timing_engine_init(&engine);
    
    uint64_t start_time = 0;
    uint64_t timeout_ns = 1000;
    
    timing_engine_advance(&engine, 500);
    
    EXPECT_FALSE(timing_engine_is_timeout(&engine, start_time, timeout_ns));
}

TEST(TimingEngineTest, TimeoutElapsed) {
    TimingEngine engine;
    timing_engine_init(&engine);
    
    uint64_t start_time = 0;
    uint64_t timeout_ns = 1000;
    
    timing_engine_advance(&engine, 1000);
    
    EXPECT_TRUE(timing_engine_is_timeout(&engine, start_time, timeout_ns));
}

TEST(TimingEngineTest, TimeoutExactlyElapsed) {
    TimingEngine engine;
    timing_engine_init(&engine);
    
    uint64_t start_time = 0;
    uint64_t timeout_ns = 1000;
    
    timing_engine_advance(&engine, 1000);
    
    EXPECT_TRUE(timing_engine_is_timeout(&engine, start_time, timeout_ns));
}

TEST(TimingEngineTest, TimeoutWithNonZeroStartTime) {
    TimingEngine engine;
    timing_engine_init(&engine);
    
    timing_engine_advance(&engine, 5000);
    uint64_t start_time = timing_engine_get_time(&engine);
    uint64_t timeout_ns = 1000;
    
    timing_engine_advance(&engine, 500);
    EXPECT_FALSE(timing_engine_is_timeout(&engine, start_time, timeout_ns));
    
    timing_engine_advance(&engine, 500);
    EXPECT_TRUE(timing_engine_is_timeout(&engine, start_time, timeout_ns));
}

TEST(TimingEngineTest, NullPointerHandling) {
    // Test that functions handle NULL pointers gracefully
    timing_engine_init(NULL);
    timing_engine_advance(NULL, 1000);
    
    EXPECT_EQ(timing_engine_get_time(NULL), 0);
    EXPECT_FALSE(timing_engine_is_timeout(NULL, 0, 1000));
}

TEST(TimingEngineTest, LargeTimeValues) {
    TimingEngine engine;
    timing_engine_init(&engine);
    
    // Test with large time values (close to uint64_t max)
    uint64_t large_delta = 1000000000000ULL; // 1 second in nanoseconds
    
    timing_engine_advance(&engine, large_delta);
    EXPECT_EQ(timing_engine_get_time(&engine), large_delta);
    
    timing_engine_advance(&engine, large_delta);
    EXPECT_EQ(timing_engine_get_time(&engine), 2 * large_delta);
}

TEST(TimingEngineTest, PropagationDelayBounds) {
    TimingEngine engine;
    timing_engine_init(&engine);
    
    // Test propagation delay within specified bounds
    uint64_t base_time = 1000;
    
    // TPROP_LOOP1: 100-190ns (recessive-to-dominant)
    uint64_t delay1_min = timing_engine_add_delay(&engine, base_time, TPROP_LOOP1_MIN_NS);
    uint64_t delay1_max = timing_engine_add_delay(&engine, base_time, TPROP_LOOP1_MAX_NS);
    
    EXPECT_EQ(delay1_min, base_time + TPROP_LOOP1_MIN_NS);
    EXPECT_EQ(delay1_max, base_time + TPROP_LOOP1_MAX_NS);
    
    // TPROP_LOOP2: 110-190ns (dominant-to-recessive)
    uint64_t delay2_min = timing_engine_add_delay(&engine, base_time, TPROP_LOOP2_MIN_NS);
    uint64_t delay2_max = timing_engine_add_delay(&engine, base_time, TPROP_LOOP2_MAX_NS);
    
    EXPECT_EQ(delay2_min, base_time + TPROP_LOOP2_MIN_NS);
    EXPECT_EQ(delay2_max, base_time + TPROP_LOOP2_MAX_NS);
}

TEST(TimingEngineTest, MicrosecondTimingParameters) {
    TimingEngine engine;
    timing_engine_init(&engine);
    
    // Test microsecond-level timing parameters
    uint64_t tpwrup_ns = TPWRUP_US * 1000; // 340μs
    uint64_t tmode1_ns = TMODE1_US * 1000; // 200μs
    uint64_t tbias_ns = TBIAS_US * 1000;   // 200μs
    
    timing_engine_advance(&engine, tpwrup_ns);
    EXPECT_EQ(timing_engine_get_time(&engine), tpwrup_ns);
    
    EXPECT_TRUE(timing_engine_is_timeout(&engine, 0, tmode1_ns));
    EXPECT_TRUE(timing_engine_is_timeout(&engine, 0, tbias_ns));
}

// ============================================================================
// Property-Based Tests
// ============================================================================

// Feature: tcan1463-q1-simulator, Property 24: Simulation time monotonicity
// **Validates: Requirements 11.3**
//
// Property: For any simulation step, when advancing simulation time,
// the new time should always be greater than or equal to the previous time.
TEST(TimingEnginePropertyTest, SimulationTimeMonotonicity) {
    rc::check("Simulation time monotonicity property", []() {
        TimingEngine engine;
        timing_engine_init(&engine);
        
        // Generate a sequence of time deltas to advance
        const auto num_steps = *rc::gen::inRange(1, 100);
        
        uint64_t previous_time = timing_engine_get_time(&engine);
        
        for (int i = 0; i < num_steps; i++) {
            // Generate a random time delta (0 to 1 second in nanoseconds)
            const auto delta_ns = *rc::gen::inRange<uint64_t>(0, 1000000000ULL);
            
            // Advance time
            timing_engine_advance(&engine, delta_ns);
            
            // Get the new time
            uint64_t current_time = timing_engine_get_time(&engine);
            
            // Property: Time should be monotonically increasing
            // (current_time >= previous_time)
            RC_ASSERT(current_time >= previous_time);
            
            // Additionally, the time should have increased by exactly delta_ns
            RC_ASSERT(current_time == previous_time + delta_ns);
            
            // Update previous_time for next iteration
            previous_time = current_time;
        }
    });
}
