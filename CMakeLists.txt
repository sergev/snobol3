cmake_minimum_required(VERSION 3.10)
project(sno CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable all warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
# Treat warnings as errors
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")

# Build library
add_library(snobol STATIC
    sno1.cpp
    sno2.cpp
    sno3.cpp
    sno4.cpp
)

# Create executable
add_executable(sno main.cpp)
target_link_libraries(sno snobol)

# Set output name
set_target_properties(sno PROPERTIES OUTPUT_NAME "sno")

add_subdirectory(tests)
