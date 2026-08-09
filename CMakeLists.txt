cmake_minimum_required(VERSION 3.20)

project(sm_td VERSION 0.6.5 LANGUAGES C)

# sm_td is compiled as part of QMK, so expose it as an interface target rather
# than attempting to build a standalone executable without QMK headers.
add_library(sm_td INTERFACE)
add_library(sm_td::sm_td ALIAS sm_td)

target_sources(sm_td INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/sm_td/sm_td.c"
)
target_include_directories(sm_td INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sm_td>"
)
target_compile_features(sm_td INTERFACE c_std_11)

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
    add_test(
        NAME sm_td.unit
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.py"
    )
    set_tests_properties(sm_td.unit PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
endif()
