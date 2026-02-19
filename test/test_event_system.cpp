#include <gtest/gtest.h>
#include "tcan1463q1_simulator.h"

// Test callback counters
static int mode_change_count = 0;
static int fault_detected_count = 0;
static int wake_up_count = 0;
static int pin_change_count = 0;
static int flag_change_count = 0;

// Test callback functions
static void mode_change_callback(const SimulatorEvent* event, void* user_data) {
    mode_change_count++;
    ASSERT_EQ(event->type, EVENT_MODE_CHANGE);
}

static void fault_detected_callback(const SimulatorEvent* event, void* user_data) {
    fault_detected_count++;
    ASSERT_EQ(event->type, EVENT_FAULT_DETECTED);
}

static void wake_up_callback(const SimulatorEvent* event, void* user_data) {
    wake_up_count++;
    ASSERT_EQ(event->type, EVENT_WAKE_UP);
}

static void pin_change_callback(const SimulatorEvent* event, void* user_data) {
    pin_change_count++;
    ASSERT_EQ(event->type, EVENT_PIN_CHANGE);
}

static void flag_change_callback(const SimulatorEvent* event, void* user_data) {
    flag_change_count++;
    ASSERT_EQ(event->type, EVENT_FLAG_CHANGE);
}

class EventSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        sim = tcan1463q1_simulator_create();
        ASSERT_NE(sim, nullptr);
        
        // Reset counters
        mode_change_count = 0;
        fault_detected_count = 0;
        wake_up_count = 0;
        pin_change_count = 0;
        flag_change_count = 0;
    }
    
    void TearDown() override {
        if (sim) {
            tcan1463q1_simulator_destroy(sim);
        }
    }
    
    TCAN1463Q1Simulator* sim;
};

TEST_F(EventSystemTest, RegisterCallback) {
    bool result = tcan1463q1_simulator_register_callback(
        sim, EVENT_MODE_CHANGE, mode_change_callback, nullptr);
    EXPECT_TRUE(result);
}

TEST_F(EventSystemTest, RegisterMultipleCallbacks) {
    bool result1 = tcan1463q1_simulator_register_callback(
        sim, EVENT_MODE_CHANGE, mode_change_callback, nullptr);
    bool result2 = tcan1463q1_simulator_register_callback(
        sim, EVENT_FAULT_DETECTED, fault_detected_callback, nullptr);
    bool result3 = tcan1463q1_simulator_register_callback(
        sim, EVENT_WAKE_UP, wake_up_callback, nullptr);
    
    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);
    EXPECT_TRUE(result3);
}

TEST_F(EventSystemTest, UnregisterCallback) {
    // Register callback
    bool reg_result = tcan1463q1_simulator_register_callback(
        sim, EVENT_MODE_CHANGE, mode_change_callback, nullptr);
    EXPECT_TRUE(reg_result);
    
    // Unregister callback
    bool unreg_result = tcan1463q1_simulator_unregister_callback(
        sim, EVENT_MODE_CHANGE, mode_change_callback);
    EXPECT_TRUE(unreg_result);
}

TEST_F(EventSystemTest, UnregisterNonExistentCallback) {
    // Try to unregister a callback that was never registered
    bool result = tcan1463q1_simulator_unregister_callback(
        sim, EVENT_MODE_CHANGE, mode_change_callback);
    EXPECT_FALSE(result);
}

TEST_F(EventSystemTest, RegisterWithNullSimulator) {
    bool result = tcan1463q1_simulator_register_callback(
        nullptr, EVENT_MODE_CHANGE, mode_change_callback, nullptr);
    EXPECT_FALSE(result);
}

TEST_F(EventSystemTest, RegisterWithNullCallback) {
    bool result = tcan1463q1_simulator_register_callback(
        sim, EVENT_MODE_CHANGE, nullptr, nullptr);
    EXPECT_FALSE(result);
}

TEST_F(EventSystemTest, RegisterWithInvalidEventType) {
    bool result = tcan1463q1_simulator_register_callback(
        sim, (SimulatorEventType)999, mode_change_callback, nullptr);
    EXPECT_FALSE(result);
}

TEST_F(EventSystemTest, UnregisterWithNullSimulator) {
    bool result = tcan1463q1_simulator_unregister_callback(
        nullptr, EVENT_MODE_CHANGE, mode_change_callback);
    EXPECT_FALSE(result);
}

TEST_F(EventSystemTest, UnregisterWithNullCallback) {
    bool result = tcan1463q1_simulator_unregister_callback(
        sim, EVENT_MODE_CHANGE, nullptr);
    EXPECT_FALSE(result);
}

TEST_F(EventSystemTest, UnregisterWithInvalidEventType) {
    bool result = tcan1463q1_simulator_unregister_callback(
        sim, (SimulatorEventType)999, mode_change_callback);
    EXPECT_FALSE(result);
}

TEST_F(EventSystemTest, RegisterMultipleCallbacksForSameEvent) {
    // Register two different callbacks for the same event type
    bool result1 = tcan1463q1_simulator_register_callback(
        sim, EVENT_MODE_CHANGE, mode_change_callback, nullptr);
    bool result2 = tcan1463q1_simulator_register_callback(
        sim, EVENT_MODE_CHANGE, fault_detected_callback, nullptr);
    
    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);
    
    // Unregister one callback
    bool unreg_result = tcan1463q1_simulator_unregister_callback(
        sim, EVENT_MODE_CHANGE, mode_change_callback);
    EXPECT_TRUE(unreg_result);
    
    // The other callback should still be registered
    // (We can't directly test this without firing events, but at least
    // we verify the unregister succeeded)
}

TEST_F(EventSystemTest, UserDataPassthrough) {
    int user_value = 42;
    
    // Register callback with user data
    bool result = tcan1463q1_simulator_register_callback(
        sim, EVENT_MODE_CHANGE, mode_change_callback, &user_value);
    EXPECT_TRUE(result);
    
    // Note: We can't test that user_data is actually passed through
    // without firing an event, but we verify registration succeeds
}
