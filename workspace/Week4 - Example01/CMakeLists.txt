cmake_minimum_required(VERSION 3.17)

# Sets all the environment variables needed to compile for the KL25Z
#
# THIS STATEMENT HAS TO BE DEFINED BEFORE THE project() COMMAND
set(CMAKE_TOOLCHAIN_FILE startup/arm_gcc.cmake)

project("CMake Week 4 - Example 1")


# Create a CMSIS library containing the register and platform specific definitions
add_library(CMSIS startup/startup_mkl25z4.c
                  CMSIS/system_MKL25Z4.c)

# Include these directories, so that the compiler can find all the header files used by the source files
include_directories(CMSIS/ startup/ inc/)


add_library(runtimestats "runtime_stats/runtime_stats.c")

target_include_directories(runtimestats PUBLIC runtime_stats/)

add_library(FreeRTOS "FreeRTOS/Source/croutine.c"
                      "FreeRTOS/Source/event_groups.c"
                      "FreeRTOS/Source/list.c"
                      "FreeRTOS/Source/queue.c"
                      "FreeRTOS/Source/stream_buffer.c"
                      "FreeRTOS/Source/tasks.c"
                      "FreeRTOS/Source/timers.c"
                      "FreeRTOS/Source/portable/GCC/ARM_CM0/port.c"
                      "FreeRTOS/Source/portable/MemMang/heap_4.c")


# FreeRTOS depends on the runtimestats library for calculating the time a task has run for.
target_link_libraries(FreeRTOS PUBLIC runtimestats)

# FreeRTOS include directories
target_include_directories(FreeRTOS PUBLIC "FreeRTOS/Source/include"
                                            "FreeRTOS/Source/portable/GCC/ARM_CM0/")

# FreeRTOS bindings are dependent on CMSIS for the register and clock definitions
target_link_libraries(FreeRTOS PUBLIC CMSIS)

# Add library for the RGB LED
add_library(rgb "rgb/rgb.c")
target_include_directories(rgb PUBLIC rgb/)

# Add library for the Serial Library
add_library(serial "serial/serial.c")
target_include_directories(serial PUBLIC serial/)

# Serial library depends on FreeRTOS
target_link_libraries(serial FreeRTOS)

# Add library for the Timer library
add_library(timer "timer/timer.c")
target_include_directories(timer PUBLIC timer/)

# Timer library depends on Serial
target_link_libraries(timer serial)

add_executable(cmake_week_4_example01.elf src/main.c)

# Link the executable with all the libraries
target_link_libraries(cmake_week_4_example01.elf PUBLIC CMSIS FreeRTOS rgb serial timer)

