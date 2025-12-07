cmake_minimum_required(VERSION 3.20)

macro(setup_test_target target_name sources)
    add_executable(${target_name} ${sources})
    set_target_properties(${target_name} PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES)
    target_include_directories(${target_name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${REACTORMQ_SOURCES_DIR}
    )
    target_link_libraries(${target_name} PRIVATE
            reactormq
            ssl
            GTest::gtest
    )

    get_target_property(_rmq_defs reactormq COMPILE_DEFINITIONS)
    get_target_property(_rmq_iface_defs reactormq INTERFACE_COMPILE_DEFINITIONS)
    if (_rmq_defs)
        target_compile_definitions(${target_name} PRIVATE ${_rmq_defs})
    endif ()
    if (_rmq_iface_defs)
        target_compile_definitions(${target_name} PRIVATE ${_rmq_iface_defs})
    endif ()

    if (MSVC)
        target_compile_options(${target_name} PRIVATE /EHsc /GR)
        target_compile_definitions(${target_name} PRIVATE _HAS_EXCEPTIONS=1)
    else ()
        target_compile_options(${target_name} PRIVATE -fexceptions -frtti)
    endif ()

    reactormq_target_warnings(${target_name})
    reactormq_add_sanitizers(${target_name})

    include(GoogleTest)
    if (WIN32 AND (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU"))
        get_filename_component(_cxx_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
        set(_env_path "${_cxx_dir};$ENV{PATH}")
        gtest_discover_tests(${target_name}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                DISCOVERY_MODE PRE_TEST
                PROPERTIES ENVIRONMENT "PATH=${_env_path}"
        )
    else ()
        gtest_discover_tests(${target_name}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                DISCOVERY_MODE PRE_TEST
        )
    endif ()
endmacro()

file(GLOB_RECURSE TEST_COMMON_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp"
)

file(GLOB_RECURSE ALL_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
if (ALL_TEST_SOURCES)
    setup_test_target(reactormq_tests "${ALL_TEST_SOURCES}")
endif ()

reactormq_disable_msan_for_targets(gtest gtest_main ssl crypto tls)

file(GLOB_RECURSE UNIT_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cpp")
if (UNIT_TEST_SOURCES)
    setup_test_target(unit_tests "${UNIT_TEST_SOURCES};${TEST_COMMON_SOURCES}")
endif ()

file(GLOB_RECURSE INTEGRATION_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/integration/*.cpp")
if (INTEGRATION_TEST_SOURCES)
    setup_test_target(integration_tests "${INTEGRATION_TEST_SOURCES};${TEST_COMMON_SOURCES}")
endif ()

setup_test_target(stress_tests "stress/test_concurrent_commands.cpp;${TEST_COMMON_SOURCES}")

# Section: Fuzz tests
if (REACTORMQ_BUILD_FUZZERS)
    if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(WARNING "REACTORMQ_BUILD_FUZZERS is ON but CMAKE_CXX_COMPILER_ID='${CMAKE_CXX_COMPILER_ID}'. libFuzzer fuzzers require Clang/AppleClang; fuzzers will not be built.")
    elseif (WIN32)
        message(WARNING "REACTORMQ_BUILD_FUZZERS is ON but Windows/MSVC is not supported for libFuzzer in this project; fuzzers will not be built.")
    else ()
        add_subdirectory(fuzz)
    endif ()
endif ()
