cmake_minimum_required(VERSION 3.14)
project(TCAN1463Q1Simulator VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(BUILD_TESTS "Build tests" ON)
option(BUILD_C_API "Build C API" ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SIMULATOR_SOURCES
    src/pin_manager.cpp
    src/mode_controller.cpp
    src/can_transceiver.cpp
    src/power_monitor.cpp
    src/fault_detector.cpp
    src/wake_handler.cpp
    src/bus_bias_controller.cpp
    src/inh_controller.cpp
    src/timing_engine.cpp
    src/simulator.cpp
    src/scenario.cpp
)

# C API sources
if(BUILD_C_API)
    list(APPEND SIMULATOR_SOURCES src/c_api.cpp)
endif()

# Create library
add_library(tcan1463q1_simulator STATIC ${SIMULATOR_SOURCES})
target_include_directories(tcan1463q1_simulator PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Testing
if(BUILD_TESTS)
    enable_testing()
    
    # Fetch RapidCheck
    include(FetchContent)
    FetchContent_Declare(
        rapidcheck
        GIT_REPOSITORY https://github.com/emil-e/rapidcheck.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(rapidcheck)
    
    # Fetch GoogleTest
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
    )
    FetchContent_MakeAvailable(googletest)
    
    # Test executable
    add_executable(tcan1463q1_tests
        test/test_main.cpp
        test/test_pin_manager.cpp
        test/test_timing_engine.cpp
        test/test_power_monitor.cpp
        test/test_mode_controller.cpp
        test/test_can_transceiver.cpp
        test/test_bus_bias_controller.cpp
        test/test_fault_detector.cpp
        test/test_wake_handler.cpp
        test/test_inh_controller.cpp
        test/test_simulator.cpp
        test/test_c_api.cpp
        test/test_event_system.cpp
    )
    
    target_link_libraries(tcan1463q1_tests
        tcan1463q1_simulator
        rapidcheck
        gtest
        gtest_main
    )
    
    add_test(NAME tcan1463q1_tests COMMAND tcan1463q1_tests)
endif()

# Installation
install(TARGETS tcan1463q1_simulator
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/ DESTINATION include)

# Examples
option(BUILD_EXAMPLES "Build example programs" ON)

if(BUILD_EXAMPLES)
    add_executable(scenario_example examples/scenario_example.cpp)
    target_link_libraries(scenario_example tcan1463q1_simulator)
    
    add_executable(event_callback_example examples/event_callback_example.cpp)
    target_link_libraries(event_callback_example tcan1463q1_simulator)
    
    # C API examples
    if(BUILD_C_API)
        add_executable(c_api_basic_example examples/c_api_basic_example.c)
        target_link_libraries(c_api_basic_example tcan1463q1_simulator)
        
        add_executable(c_api_error_handling_example examples/c_api_error_handling_example.c)
        target_link_libraries(c_api_error_handling_example tcan1463q1_simulator)
    endif()
endif()
