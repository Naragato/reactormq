cmake_minimum_required(VERSION 3.20)

macro(add_fuzzer_target target_name source_file)
    add_executable(${target_name} ${source_file})

    set_target_properties(${target_name} PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO
    )

    target_include_directories(${target_name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${REACTORMQ_SOURCES_DIR}
    )

    target_link_libraries(${target_name} PRIVATE
            reactormq
            ssl
    )

    get_target_property(_rmq_defs reactormq COMPILE_DEFINITIONS)
    get_target_property(_rmq_iface_defs reactormq INTERFACE_COMPILE_DEFINITIONS)
    if (_rmq_defs)
        target_compile_definitions(${target_name} PRIVATE ${_rmq_defs})
    endif ()
    if (_rmq_iface_defs)
        target_compile_definitions(${target_name} PRIVATE ${_rmq_iface_defs})
    endif ()

    reactormq_target_warnings(${target_name})
    reactormq_add_fuzzer_sanitizers(${target_name})

    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/corpus/${target_name})

    message(STATUS "Added fuzzer target: ${target_name}")
endmacro()

add_fuzzer_target(fuzz_mqtt_codec fuzz_mqtt_codec.cpp)
add_fuzzer_target(fuzz_fixed_header fuzz_fixed_header.cpp)
add_fuzzer_target(fuzz_variable_byte_integer fuzz_variable_byte_integer.cpp)
add_fuzzer_target(fuzz_publish_packet fuzz_publish_packet.cpp)
add_fuzzer_target(fuzz_connect_packet fuzz_connect_packet.cpp)
