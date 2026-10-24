cmake_minimum_required(VERSION 3.20)

project(stmboot_host_tests NONE)

find_program(BASH_EXECUTABLE bash REQUIRED)

add_custom_target(unit_tests
    COMMAND ceedling clobber
    COMMAND ceedling test:all
    COMMAND ceedling gcov:all
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unit
    COMMENT "Running Ceedling unit tests and coverage"
    VERBATIM
)

add_custom_target(integration_tests
    COMMAND ceedling clobber
    COMMAND ceedling test:all
    COMMAND ceedling gcov:all
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/integration
    COMMENT "Running Ceedling integration tests and coverage"
    VERBATIM
)

add_custom_target(static_analysis
    COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/static/run_static_checks.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
    COMMENT "Running cppcheck, clang-tidy, and clang-format"
    VERBATIM
)

add_custom_target(all_tests
    DEPENDS unit_tests integration_tests static_analysis
    COMMENT "All host tests and static analysis"
)
