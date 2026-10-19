cmake_minimum_required(VERSION 3.20)
project(production_cpp_challenge CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Optimization for fast compile times in containers
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

find_package(Threads REQUIRED)
include_directories(include)

# 1. Core library with candidate-editable services
file(GLOB_RECURSE CORE_SOURCES "src/service/*.cpp")
add_library(core_services STATIC ${CORE_SOURCES})

# 2. Main Web Server Target (name MUST match what start.sh executes)
add_executable(server src/main.cpp)
target_link_libraries(server PRIVATE core_services Threads::Threads)
if(WIN32)
  target_link_libraries(server PRIVATE ws2_32 crypt32)
endif()

# 3. Test Runner Target (name MUST match what tests/run_tests.sh executes)
add_executable(challenge_tests tests/test_suite.cpp)
target_link_libraries(challenge_tests PRIVATE core_services)
