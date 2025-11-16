cmake_minimum_required(VERSION 3.15)
project(CurveBug C)

set(CMAKE_C_STANDARD 11)

# raylib
add_subdirectory(external/raylib)

# raygui is header-only, just needs to be in include path
include_directories(external/raylib/src)
include_directories(external/raygui/src)

add_executable(curvebug
    src/main.c
    src/serial.c
    src/config.c
)

target_link_libraries(curvebug raylib)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(curvebug winmm)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(curvebug m pthread dl)
endif()