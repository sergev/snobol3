cmake_minimum_required(VERSION 3.10)
project(sno C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Enable all warnings
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
# Treat warnings as errors
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")

# Build library
add_library(snobol STATIC
    sno1.c
    sno2.c
    sno3.c
    sno4.c
)

# Create executable
add_executable(sno main.c)
target_link_libraries(sno snobol)

# Set output name
set_target_properties(sno PROPERTIES OUTPUT_NAME "sno")

add_subdirectory(tests)
