cmake_minimum_required(VERSION 4.00)
project(reactormq LANGUAGES CXX)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(Platform)
include(Engine)

reactormq_set_defaults()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler Detection

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(REACTORMQ_COMPILER_MSVC TRUE CACHE INTERNAL "Compiler is MSVC")
    set(REACTORMQ_COMPILER_NAME "MSVC" CACHE INTERNAL "Compiler name")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(REACTORMQ_COMPILER_GCC TRUE CACHE INTERNAL "Compiler is GCC")
    set(REACTORMQ_COMPILER_NAME "GCC" CACHE INTERNAL "Compiler name")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(REACTORMQ_COMPILER_CLANG TRUE CACHE INTERNAL "Compiler is Clang")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        set(REACTORMQ_COMPILER_APPLE_CLANG TRUE CACHE INTERNAL "Compiler is AppleClang")
        set(REACTORMQ_COMPILER_NAME "AppleClang" CACHE INTERNAL "Compiler name")
    else ()
        set(REACTORMQ_COMPILER_NAME "Clang" CACHE INTERNAL "Compiler name")
    endif ()
else ()
    set(REACTORMQ_COMPILER_NAME "${CMAKE_CXX_COMPILER_ID}" CACHE INTERNAL "Compiler name")
endif ()

message(STATUS "Detected compiler: ${REACTORMQ_COMPILER_NAME}")

# Section: Platform Detection

reactormq_detect_platform(REACTORMQ_PLATFORM_FAMILY REACTORMQ_PLATFORM_NAME)

message(STATUS "Platform: ${REACTORMQ_PLATFORM_NAME}")
message(STATUS "Family: ${REACTORMQ_PLATFORM_FAMILY}")

# Section: Engine Detection

reactormq_detect_engine(REACTORMQ_ENGINE_NAME)

include(CheckSymbolExists)
check_symbol_exists(gethostname "unistd.h" REACTORMQ_WITH_GETHOSTNAME)
check_symbol_exists(uname "sys/utsname.h" REACTORMQ_WITH_UNAME)

# Section: Sources

set(REACTORMQ_SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

file(GLOB_RECURSE REACTORMQ_SOURCES
    ${REACTORMQ_SOURCES_DIR}/**.cpp
)
list(FILTER REACTORMQ_SOURCES EXCLUDE REGEX ".*src/socket/ue5/tests/.*\\.cpp$")

file(GLOB_RECURSE REACTORMQ_PRIVATE_HEADERS
    ${REACTORMQ_SOURCES_DIR}/**.h
    ${REACTORMQ_SOURCES_DIR}/**.hpp
)

# NO UE5 within CMake Builds
list(FILTER REACTORMQ_SOURCES EXCLUDE REGEX ".*ue5_websocket\\.cpp$")
list(FILTER REACTORMQ_PRIVATE_HEADERS EXCLUDE REGEX ".*ue5_websocket\\.h$")
list(FILTER REACTORMQ_SOURCES EXCLUDE REGEX ".*ue5_socket\\.cpp$")
list(FILTER REACTORMQ_PRIVATE_HEADERS EXCLUDE REGEX ".*ue5_socket\\.h$")

# Filter out ALL platform-specific socket sources
# They will be added back based on platform/family/engine configuration
list(FILTER REACTORMQ_SOURCES EXCLUDE REGEX ".*src/socket/platform/(win|posix|sony)_socket\\.cpp$")
list(FILTER REACTORMQ_SOURCES EXCLUDE REGEX ".*src/socket/platform/(native|bio)_secure_socket\\.cpp$")

set(REACTORMQ_PUBLIC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
file(GLOB_RECURSE REACTORMQ_PUBLIC_HEADERS
    ${REACTORMQ_PUBLIC_INCLUDE_DIR}/reactormq/*.h
    ${REACTORMQ_PUBLIC_INCLUDE_DIR}/reactormq/*.hpp
)

# Section: Library Target

if (COMMAND ly_add_target)
    ly_add_target(
            NAME reactormq STATIC
            NAMESPACE reactormq
            FILES_CMAKE
            ${REACTORMQ_SOURCES}
            ${REACTORMQ_PRIVATE_HEADERS}
            ${REACTORMQ_PUBLIC_HEADERS}
            INCLUDE_DIRECTORIES
            PUBLIC
            ${REACTORMQ_PUBLIC_INCLUDE_DIR}
            PRIVATE
            ${REACTORMQ_SOURCES_DIR}
            BUILD_DEPENDENCIES
            PUBLIC
            AZ::AzCore
            AZ::AzNetworking
    )
else ()
    if (REACTORMQ_BUILD_SHARED)
        add_library(reactormq SHARED
                ${REACTORMQ_SOURCES}
                ${REACTORMQ_PRIVATE_HEADERS}
                ${REACTORMQ_PUBLIC_HEADERS}
        )
        target_compile_definitions(reactormq PRIVATE REACTORMQ_BUILD_SHARED=1)
    else ()
        add_library(reactormq STATIC
                ${REACTORMQ_SOURCES}
                ${REACTORMQ_PRIVATE_HEADERS}
                ${REACTORMQ_PUBLIC_HEADERS}
        )
        target_compile_definitions(reactormq PUBLIC REACTORMQ_BUILD_STATIC=1)
    endif ()

    set_target_properties(reactormq PROPERTIES
            POSITION_INDEPENDENT_CODE ON
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO
    )

    target_include_directories(reactormq
            PUBLIC
            ${REACTORMQ_PUBLIC_INCLUDE_DIR}
            PRIVATE
            ${REACTORMQ_SOURCES_DIR}
    )
endif ()

# Section: Platform Configuration

reactormq_apply_family(reactormq ${REACTORMQ_PLATFORM_FAMILY})
reactormq_apply_platform(reactormq ${REACTORMQ_PLATFORM_NAME})

# Section: Engine Configuration

reactormq_apply_engine(reactormq ${REACTORMQ_ENGINE_NAME})

# Section: Socket Source Selection

# Section: OpenSSL Integration

include(cmake/SSL.cmake)
reactormq_configure_ssl()

reactormq_collect_socket_sources(REACTORMQ_SOCKET_SOURCES ${REACTORMQ_SOURCES_DIR})
if (REACTORMQ_SOCKET_SOURCES)
    target_sources(reactormq PRIVATE ${REACTORMQ_SOCKET_SOURCES})
    message(STATUS "Added socket sources: ${REACTORMQ_SOCKET_SOURCES}")
endif ()

# Section: Target Configuration

reactormq_target_warnings(reactormq)
reactormq_target_features(reactormq)
reactormq_target_sockets(reactormq)
reactormq_target_ssl(reactormq)
reactormq_add_sanitizers(reactormq)

target_compile_definitions(reactormq PRIVATE
    REACTORMQ_WITH_EXCEPTIONS=$<BOOL:${REACTORMQ_WITH_EXCEPTIONS}>
    REACTORMQ_WITH_THREADS=$<BOOL:${REACTORMQ_WITH_THREADS}>
    REACTORMQ_WITH_CONSOLE_SINK=$<BOOL:${REACTORMQ_WITH_CONSOLE_SINK}>
    REACTORMQ_WITH_FILE_SINK=$<BOOL:${REACTORMQ_WITH_FILE_SINK}>
    REACTORMQ_WITH_UE5=$<BOOL:${REACTORMQ_ENABLE_UE5}>
    REACTORMQ_WITH_O3DE=$<BOOL:${REACTORMQ_WITH_O3DE}>
    REACTORMQ_WITH_GETHOSTNAME=$<BOOL:${REACTORMQ_WITH_GETHOSTNAME}>
    REACTORMQ_WITH_UNAME=$<BOOL:${REACTORMQ_WITH_UNAME}>
)

# Section: Tests & Fuzzing

reactormq_configure_tests_for_platform()