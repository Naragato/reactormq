cmake_minimum_required(VERSION 3.20)

# Sources
file(GLOB_RECURSE REACTORMQ_TEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
if (NOT REACTORMQ_TEST_SOURCES)
    message(WARNING "No test sources found under ${CMAKE_CURRENT_SOURCE_DIR}")
    return()
endif ()

# Target
add_executable(reactormq_tests ${REACTORMQ_TEST_SOURCES})
set_target_properties(reactormq_tests PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES)
target_include_directories(reactormq_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
target_link_libraries(reactormq_tests PRIVATE
        reactormq
        ssl
        GTest::gtest
)

# Definitions
get_target_property(_rmq_defs reactormq COMPILE_DEFINITIONS)
get_target_property(_rmq_iface_defs reactormq INTERFACE_COMPILE_DEFINITIONS)
if (_rmq_defs)
    target_compile_definitions(reactormq_tests PRIVATE ${_rmq_defs})
endif ()
if (_rmq_iface_defs)
    target_compile_definitions(reactormq_tests PRIVATE ${_rmq_iface_defs})
endif ()

# Options
if (MSVC)
    target_compile_options(reactormq_tests PRIVATE /EHsc /GR)
    target_compile_definitions(reactormq_tests PRIVATE _HAS_EXCEPTIONS=1)
else ()
    target_compile_options(reactormq_tests PRIVATE -fexceptions -frtti)
endif ()

# Discovery
include(GoogleTest)
if (WIN32 AND (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU"))
    get_filename_component(_cxx_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set(_env_path "${_cxx_dir};$ENV{PATH}")
    gtest_discover_tests(reactormq_tests
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            DISCOVERY_MODE PRE_TEST
            PROPERTIES ENVIRONMENT "PATH=${_env_path}"
    )
else ()
    gtest_discover_tests(reactormq_tests
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            DISCOVERY_MODE PRE_TEST
    )
endif ()
