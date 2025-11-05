cmake_minimum_required(VERSION 3.23)
project(reactormq LANGUAGES CXX)

# --------------------------------------------------------------------------------------
# Language / standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# --------------------------------------------------------------------------------------
# Platform Detection
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(REACTORMQ_PLATFORM_WINDOWS TRUE CACHE INTERNAL "Target platform is Windows")
    set(REACTORMQ_PLATFORM_NAME "Windows" CACHE INTERNAL "Target platform name")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(REACTORMQ_PLATFORM_LINUX TRUE CACHE INTERNAL "Target platform is Linux")
    set(REACTORMQ_PLATFORM_NAME "Linux" CACHE INTERNAL "Target platform name")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    string(TOLOWER "${CMAKE_OSX_SYSROOT}" _osx_sysroot_lower)
    if (NOT CMAKE_OSX_SYSROOT OR _osx_sysroot_lower MATCHES "macosx")
        set(REACTORMQ_PLATFORM_MACOS TRUE CACHE INTERNAL "Target platform is macOS")
        set(REACTORMQ_PLATFORM_NAME "macOS" CACHE INTERNAL "Target platform name")
    elseif (_osx_sysroot_lower MATCHES "iphoneos")
        set(REACTORMQ_PLATFORM_IOS TRUE CACHE INTERNAL "Target platform is iOS")
        set(REACTORMQ_PLATFORM_NAME "iOS" CACHE INTERNAL "Target platform name")
    else ()
        set(REACTORMQ_PLATFORM_MACOS TRUE CACHE INTERNAL "Target platform is macOS (default)")
        set(REACTORMQ_PLATFORM_NAME "macOS" CACHE INTERNAL "Target platform name")
    endif ()
    unset(_osx_sysroot_lower)
elseif (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(REACTORMQ_PLATFORM_IOS TRUE CACHE INTERNAL "Target platform is iOS")
    set(REACTORMQ_PLATFORM_NAME "iOS" CACHE INTERNAL "Target platform name")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(REACTORMQ_PLATFORM_ANDROID TRUE CACHE INTERNAL "Target platform is Android")
    set(REACTORMQ_PLATFORM_NAME "Android" CACHE INTERNAL "Target platform name")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Prospero")
    set(REACTORMQ_PLATFORM_PROSPERO TRUE CACHE INTERNAL "Target platform is PlayStation 5")
    set(REACTORMQ_PLATFORM_NAME "Prospero" CACHE INTERNAL "Target platform name")
elseif (CMAKE_SYSTEM_NAME MATCHES "Gaming\\.Xbox\\.Scarlett\\.x64")
    set(REACTORMQ_PLATFORM_SCARLETT TRUE CACHE INTERNAL "Target platform is Xbox Series X|S")
    set(REACTORMQ_PLATFORM_NAME "Scarlett" CACHE INTERNAL "Target platform name")
elseif (CMAKE_SYSTEM_NAME STREQUAL "NX")
    set(REACTORMQ_PLATFORM_NX TRUE CACHE INTERNAL "Target platform is Nintendo Switch")
    set(REACTORMQ_PLATFORM_NAME "NX" CACHE INTERNAL "Target platform name")
else ()
    set(REACTORMQ_PLATFORM_NAME "Unknown" CACHE INTERNAL "Target platform name")
    message(WARNING "Unrecognized platform: ${CMAKE_SYSTEM_NAME}. Proceeding with defaults.")
endif ()

if (REACTORMQ_PLATFORM_WINDOWS OR REACTORMQ_PLATFORM_SCARLETT)
    set(REACTORMQ_PLATFORM_WINDOWS_FAMILY TRUE CACHE INTERNAL "Target is part of Windows family")
endif ()

if (REACTORMQ_PLATFORM_MACOS OR REACTORMQ_PLATFORM_IOS)
    set(REACTORMQ_PLATFORM_DARWIN_FAMILY TRUE CACHE INTERNAL "Target is part of Darwin/Apple family")
endif ()

if (REACTORMQ_PLATFORM_LINUX OR REACTORMQ_PLATFORM_ANDROID OR REACTORMQ_PLATFORM_DARWIN_FAMILY OR REACTORMQ_PLATFORM_PROSPERO OR REACTORMQ_PLATFORM_NX)
    set(REACTORMQ_PLATFORM_POSIX_FAMILY TRUE CACHE INTERNAL "Target is POSIX-compatible")
endif ()

if (REACTORMQ_PLATFORM_PROSPERO OR REACTORMQ_PLATFORM_SCARLETT OR REACTORMQ_PLATFORM_NX)
    set(REACTORMQ_PLATFORM_CONSOLE_FAMILY TRUE CACHE INTERNAL "Target is a gaming console")
endif ()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64|x64")
    set(REACTORMQ_ARCH "x64" CACHE INTERNAL "Target architecture")
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64|arm64")
    set(REACTORMQ_ARCH "ARM64" CACHE INTERNAL "Target architecture")
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    set(REACTORMQ_ARCH "ARM" CACHE INTERNAL "Target architecture")
else ()
    set(REACTORMQ_ARCH "${CMAKE_SYSTEM_PROCESSOR}" CACHE INTERNAL "Target architecture")
endif ()

message(STATUS "Detected platform: ${REACTORMQ_PLATFORM_NAME} (${REACTORMQ_ARCH})")
if (REACTORMQ_PLATFORM_WINDOWS_FAMILY)
    message(STATUS "  Platform family: Windows")
elseif (REACTORMQ_PLATFORM_DARWIN_FAMILY)
    message(STATUS "  Platform family: Darwin/Apple")
elseif (REACTORMQ_PLATFORM_POSIX_FAMILY)
    message(STATUS "  Platform family: POSIX")
endif ()
if (REACTORMQ_PLATFORM_CONSOLE_FAMILY)
    message(STATUS "  Console platform detected")
endif ()

# --------------------------------------------------------------------------------------
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

# --------------------------------------------------------------------------------------
# Options
option(REACTORMQ_BUILD_SHARED "Build shared library" OFF)
option(REACTORMQ_BUILD_TESTS "Build tests" ON)
option(REACTORMQ_WITH_THREADS "Enable threading support" ON)

set(_DEF_WITH_WIN_SOCKETS OFF)
set(_DEF_WITH_POSIX_SOCKETS OFF)
if (REACTORMQ_PLATFORM_WINDOWS_FAMILY)
    set(_DEF_WITH_WIN_SOCKETS ON)
endif ()
if (REACTORMQ_PLATFORM_POSIX_FAMILY)
    set(_DEF_WITH_POSIX_SOCKETS ON)
endif ()
option(REACTORMQ_WITH_WIN_SOCKETS "Enable Windows socket backend" ${_DEF_WITH_WIN_SOCKETS})
option(REACTORMQ_WITH_POSIX_SOCKETS "Enable POSIX socket backend" ${_DEF_WITH_POSIX_SOCKETS})
option(REACTORMQ_WITH_ANDROID_SOCKETS "Enable Android socket backend" OFF)
option(REACTORMQ_WITH_DARWIN_SOCKETS "Enable Darwin (macOS/iOS) socket backend" OFF)
option(REACTORMQ_WITH_SWITCH_SOCKETS "Enable Nintendo Switch socket backend" OFF)
option(REACTORMQ_WITH_SONY_SOCKETS "Enable Sony PlayStation socket backend" OFF)

option(REACTORMQ_WITH_WEBSOCKET_UE5 "Enable UE5 WebSocket adapter" OFF)
option(REACTORMQ_WITH_FSOCKET_UE5 "Enable UE5 FSocket adapter" OFF)
option(REACTORMQ_WITH_UE_LOGGER "Enable Unreal Engine logging sink" OFF)

if (COMMAND ly_add_target)
    set(REACTORMQ_IN_O3DE_GEM TRUE CACHE INTERNAL "Building as O3DE Gem")
    message(STATUS "Detected O3DE Gem context (ly_add_target command available)")
else ()
    set(REACTORMQ_IN_O3DE_GEM FALSE CACHE INTERNAL "Not building as O3DE Gem")
endif ()

if (REACTORMQ_IN_O3DE_GEM)
    option(REACTORMQ_WITH_O3DE_SOCKET "Enable O3DE socket adapter" ON)
else ()
    option(REACTORMQ_WITH_O3DE_SOCKET "Enable O3DE socket adapter" OFF)
endif ()

option(REACTORMQ_WITH_EXCEPTIONS "Enable C++ exceptions" OFF)
option(REACTORMQ_WITH_RTTI "Enable RTTI (typeid/dynamic_cast)" OFF)
option(REACTORMQ_WITH_CONSOLE_SINK "Enable built-in console log sink" ON)
option(REACTORMQ_WITH_FILE_SINK "Enable built-in file log sink" ON)

set(REACTORMQ_OPENSSL_PROVIDER "awslc" CACHE STRING "TLS provider (system|awslc|none)")
set_property(CACHE REACTORMQ_OPENSSL_PROVIDER PROPERTY STRINGS system awslc none)
option(REACTORMQ_OPENSSL_USE_SHARED_LIBS "Prefer static OpenSSL libraries" OFF)

# --------------------------------------------------------------------------------------
# OpenSSL Integration
set(REACTORMQ_OPENSSL_AVAILABLE FALSE)

if (REACTORMQ_WITH_WEBSOCKET_UE5 OR REACTORMQ_WITH_FSOCKET_UE5 OR REACTORMQ_WITH_UE_LOGGER)
    message(STATUS "UE5 context detected - OpenSSL handled by Unreal Build Tool")
    set(REACTORMQ_OPENSSL_AVAILABLE TRUE)
elseif (REACTORMQ_IN_O3DE_GEM)
    message(STATUS "O3DE Gem context - OpenSSL will be provided by O3DE dependencies")
    set(REACTORMQ_OPENSSL_AVAILABLE TRUE)
elseif (REACTORMQ_OPENSSL_PROVIDER STREQUAL "awslc")
    include(FetchContent)

    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(BUILD_TOOL OFF CACHE BOOL "" FORCE)
    set(BUILD_LIBSSL ON CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(DISABLE_PERL ON CACHE BOOL "" FORCE)
    set(DISABLE_GO ON CACHE BOOL "" FORCE)

    FetchContent_Declare(
            awslc
            GIT_REPOSITORY https://github.com/aws/aws-lc.git
            GIT_TAG v1.64.0
    )

    FetchContent_MakeAvailable(awslc)

    include(FetchContent)

    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(BUILD_TOOL OFF CACHE BOOL "" FORCE)
    set(BUILD_LIBSSL ON CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(DISABLE_PERL ON CACHE BOOL "" FORCE)
    set(DISABLE_GO ON CACHE BOOL "" FORCE)

    FetchContent_Declare(
            awslc
            GIT_REPOSITORY https://github.com/aws/aws-lc.git
            GIT_TAG v1.64.0
    )

    FetchContent_MakeAvailable(awslc)

    if (MSVC)
        if (TARGET crypto)
            target_compile_options(crypto PRIVATE /wd5262)
        endif ()
        if (TARGET ssl)
            target_compile_options(ssl PRIVATE /wd5262)
        endif ()
    endif ()

    add_library(OpenSSL::Crypto ALIAS crypto)
    add_library(OpenSSL::SSL ALIAS ssl)
    set(REACTORMQ_OPENSSL_AVAILABLE TRUE)

elseif (REACTORMQ_OPENSSL_PROVIDER STREQUAL "system")
    if (NOT REACTORMQ_OPENSSL_USE_SHARED_LIBS)
        set(OPENSSL_USE_STATIC_LIBS TRUE)
    endif ()

    function(_rmq_has_msvc_ossl prefix out)
        if (EXISTS "${prefix}/lib/ssl.lib" AND EXISTS "${prefix}/lib/crypto.lib")
            set(${out} TRUE PARENT_SCOPE)
        else ()
            set(${out} FALSE PARENT_SCOPE)
        endif ()
    endfunction()
    function(_rmq_has_mingw_ossl prefix out)
        if (EXISTS "${prefix}/lib/libssl.a" AND EXISTS "${prefix}/lib/libcrypto.a")
            set(${out} TRUE PARENT_SCOPE)
        else ()
            set(${out} FALSE PARENT_SCOPE)
        endif ()
    endfunction()

    if (DEFINED OPENSSL_ROOT_DIR)
        if (MSVC)
            _rmq_has_msvc_ossl("${OPENSSL_ROOT_DIR}" _ok)
            if (NOT _ok)
                message(WARNING "OPENSSL_ROOT_DIR='${OPENSSL_ROOT_DIR}' does not contain MSVC libs (ssl.lib/crypto.lib). Ignoring.")
                unset(OPENSSL_ROOT_DIR CACHE)
            endif ()
        elseif (MINGW)
            _rmq_has_mingw_ossl("${OPENSSL_ROOT_DIR}" _ok)
            if (NOT _ok)
                message(WARNING "OPENSSL_ROOT_DIR='${OPENSSL_ROOT_DIR}' does not contain MinGW libs (libssl.a/libcrypto.a). Ignoring.")
                unset(OPENSSL_ROOT_DIR CACHE)
            endif ()
        endif ()
    endif ()

    if (NOT DEFINED OPENSSL_ROOT_DIR)
        if (MSVC AND NOT CMAKE_CROSSCOMPILING)
            foreach (_pf IN ITEMS "$ENV{ProgramFiles}" "$ENV{ProgramFiles\(x86\)}")
                if (EXISTS "${_pf}/OpenSSL")
                    _rmq_has_msvc_ossl("${_pf}/OpenSSL" _ok)
                    if (_ok)
                        set(OPENSSL_ROOT_DIR "${_pf}/OpenSSL" CACHE PATH "OpenSSL root (MSVC)" FORCE)
                        message(STATUS "Using MSVC OpenSSL at: ${OPENSSL_ROOT_DIR}")
                        break()
                    endif ()
                endif ()
            endforeach ()
        elseif (MINGW)
            get_filename_component(_ccdir "${CMAKE_C_COMPILER}" DIRECTORY)
            if (_ccdir MATCHES "/bin$")
                string(REGEX REPLACE "/bin$" "" _prefix "${_ccdir}")
                _rmq_has_mingw_ossl("${_prefix}" _ok)
                if (_ok)
                    set(OPENSSL_ROOT_DIR "${_prefix}" CACHE PATH "OpenSSL root (MinGW)" FORCE)
                    message(STATUS "Using MinGW/MSYS2 OpenSSL at: ${OPENSSL_ROOT_DIR}")
                endif ()
            endif ()
            if (NOT DEFINED OPENSSL_ROOT_DIR AND DEFINED ENV{MINGW_PREFIX})
                _rmq_has_mingw_ossl("$ENV{MINGW_PREFIX}" _ok)
                if (_ok)
                    set(OPENSSL_ROOT_DIR "$ENV{MINGW_PREFIX}" CACHE PATH "OpenSSL root (MinGW)" FORCE)
                    message(STATUS "Using MinGW OpenSSL from MINGW_PREFIX: ${OPENSSL_ROOT_DIR}")
                endif ()
            endif ()
        endif ()
    endif ()

    if (CMAKE_CROSSCOMPILING AND NOT DEFINED OPENSSL_ROOT_DIR)
        message(STATUS "Cross-compiling: relying on toolchain/sysroot to locate OpenSSL")
    endif ()

    set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)
    find_package(OpenSSL REQUIRED)

    if (OpenSSL_FOUND)
        set(REACTORMQ_OPENSSL_AVAILABLE TRUE)
        message(STATUS "Found OpenSSL ${OPENSSL_VERSION}")
        if (DEFINED OPENSSL_INCLUDE_DIR)
            message(STATUS "  Include dir: ${OPENSSL_INCLUDE_DIR}")
        endif ()
        if (DEFINED OPENSSL_SSL_LIBRARY)
            message(STATUS "  SSL library: ${OPENSSL_SSL_LIBRARY}")
        endif ()
        if (DEFINED OPENSSL_CRYPTO_LIBRARY)
            message(STATUS "  Crypto library: ${OPENSSL_CRYPTO_LIBRARY}")
        endif ()
    endif ()
endif ()

if (REACTORMQ_OPENSSL_AVAILABLE)
    message(STATUS "OpenSSL integration: ENABLED")
else ()
    message(STATUS "OpenSSL integration: DISABLED (native socket TLS features will be unavailable)")
endif ()

# --------------------------------------------------------------------------------------
# Sources
file(GLOB_RECURSE REACTORMQ_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp
)
list(FILTER REACTORMQ_SOURCES EXCLUDE REGEX ".*src/socket/ue5/tests/.*\\.cpp$")

file(GLOB_RECURSE REACTORMQ_PRIVATE_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/**.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/**.hpp
)

if (NOT REACTORMQ_WITH_WEBSOCKET_UE5)
    list(FILTER REACTORMQ_SOURCES EXCLUDE REGEX ".*ue5_websocket\\.cpp$")
    list(FILTER REACTORMQ_PRIVATE_HEADERS EXCLUDE REGEX ".*ue5_websocket\\.h$")
endif ()
if (NOT REACTORMQ_WITH_FSOCKET_UE5)
    list(FILTER REACTORMQ_SOURCES EXCLUDE REGEX ".*ue5_socket\\.cpp$")
    list(FILTER REACTORMQ_PRIVATE_HEADERS EXCLUDE REGEX ".*ue5_socket\\.h$")
endif ()

if (NOT REACTORMQ_WITH_O3DE_SOCKET)
    list(FILTER REACTORMQ_SOURCES EXCLUDE REGEX ".*o3de_socket\\.cpp$")
    list(FILTER REACTORMQ_PRIVATE_HEADERS EXCLUDE REGEX ".*o3de_socket\\.h$")
endif ()

if (NOT REACTORMQ_WITH_WIN_SOCKETS)
    list(FILTER REACTORMQ_SOURCES EXCLUDE REGEX ".*win_socket\\.cpp$")
endif ()
if (NOT REACTORMQ_WITH_POSIX_SOCKETS)
    list(FILTER REACTORMQ_SOURCES EXCLUDE REGEX ".*posix_socket\\.cpp$")
endif ()

set(REACTORMQ_PUBLIC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
file(GLOB_RECURSE REACTORMQ_PUBLIC_HEADERS
        ${REACTORMQ_PUBLIC_INCLUDE_DIR}/reactormq/*.h
        ${REACTORMQ_PUBLIC_INCLUDE_DIR}/reactormq/*.hpp
)

# --------------------------------------------------------------------------------------
# Library target - Dual mode: O3DE Gem vs Standalone
if (REACTORMQ_IN_O3DE_GEM)
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
            ${CMAKE_CURRENT_SOURCE_DIR}/src
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
    else ()
        add_library(reactormq STATIC
                ${REACTORMQ_SOURCES}
                ${REACTORMQ_PRIVATE_HEADERS}
                ${REACTORMQ_PUBLIC_HEADERS}
        )
    endif ()

    if (REACTORMQ_BUILD_SHARED)
        target_compile_definitions(reactormq PRIVATE REACTORMQ_BUILD=1)
    else ()
        target_compile_definitions(reactormq PUBLIC REACTORMQ_STATIC=1)
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
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
endif ()

# --------------------------------------------------------------------------------------
# Compile definitions
target_compile_definitions(reactormq
        PRIVATE
        $<$<BOOL:${REACTORMQ_WITH_THREADS}>:REACTORMQ_WITH_THREADS=1>
        $<$<BOOL:${REACTORMQ_WITH_POSIX_SOCKETS}>:REACTORMQ_WITH_POSIX_SOCKETS=1>
        $<$<BOOL:${REACTORMQ_WITH_WIN_SOCKETS}>:REACTORMQ_WITH_WIN_SOCKETS=1>
        $<$<BOOL:${REACTORMQ_WITH_ANDROID_SOCKETS}>:REACTORMQ_WITH_ANDROID_SOCKETS=1;REACTORMQ_WITH_POSIX_SOCKETS=1>
        $<$<BOOL:${REACTORMQ_WITH_DARWIN_SOCKETS}>:REACTORMQ_WITH_DARWIN_SOCKETS=1;REACTORMQ_WITH_POSIX_SOCKETS=1>
        $<$<BOOL:${REACTORMQ_WITH_SWITCH_SOCKETS}>:REACTORMQ_WITH_SWITCH_SOCKETS=1>
        $<$<BOOL:${REACTORMQ_WITH_SONY_SOCKETS}>:REACTORMQ_WITH_SONY_SOCKETS=1>
        $<$<BOOL:${REACTORMQ_WITH_WEBSOCKET_UE5}>:REACTORMQ_WITH_WEBSOCKET_UE5=1>
        $<$<BOOL:${REACTORMQ_WITH_FSOCKET_UE5}>:REACTORMQ_WITH_FSOCKET_UE5=1>
        $<$<BOOL:${REACTORMQ_WITH_O3DE_SOCKET}>:REACTORMQ_WITH_O3DE_SOCKET=1>
        $<$<BOOL:${REACTORMQ_WITH_UE_LOGGER}>:REACTORMQ_WITH_UE_LOGGER=1>
        $<$<BOOL:${REACTORMQ_WITH_CONSOLE_SINK}>:REACTORMQ_WITH_CONSOLE_SINK=1>
        $<$<BOOL:${REACTORMQ_WITH_FILE_SINK}>:REACTORMQ_WITH_FILE_SINK=1>
        $<$<BOOL:${REACTORMQ_OPENSSL_AVAILABLE}>:REACTORMQ_WITH_OPENSSL=1>

        $<$<BOOL:${REACTORMQ_PLATFORM_WINDOWS}>:REACTORMQ_PLATFORM_WINDOWS=1>
        $<$<BOOL:${REACTORMQ_PLATFORM_LINUX}>:REACTORMQ_PLATFORM_LINUX=1>
        $<$<BOOL:${REACTORMQ_PLATFORM_ANDROID}>:REACTORMQ_PLATFORM_ANDROID=1>
        $<$<BOOL:${REACTORMQ_PLATFORM_MACOS}>:REACTORMQ_PLATFORM_MACOS=1>
        $<$<BOOL:${REACTORMQ_PLATFORM_IOS}>:REACTORMQ_PLATFORM_IOS=1>
        $<$<BOOL:${REACTORMQ_PLATFORM_WINDOWS_FAMILY}>:REACTORMQ_PLATFORM_WINDOWS_FAMILY=1;WIN32_LEAN_AND_MEAN;NOMINMAX>
        $<$<BOOL:${REACTORMQ_PLATFORM_DARWIN_FAMILY}>:REACTORMQ_PLATFORM_DARWIN_FAMILY=1>
        $<$<BOOL:${REACTORMQ_PLATFORM_POSIX_FAMILY}>:REACTORMQ_PLATFORM_POSIX_FAMILY=1>
        $<$<BOOL:${REACTORMQ_PLATFORM_CONSOLE_FAMILY}>:REACTORMQ_PLATFORM_CONSOLE_FAMILY=1>
)

target_link_libraries(reactormq
        PRIVATE
        $<$<AND:$<BOOL:${REACTORMQ_PLATFORM_WINDOWS_FAMILY}>,$<BOOL:${REACTORMQ_PLATFORM_WINDOWS}>>:ws2_32>
)

target_link_options(reactormq
        PRIVATE
        $<$<AND:$<BOOL:${REACTORMQ_PLATFORM_DARWIN_FAMILY}>,$<BOOL:${REACTORMQ_OPENSSL_AVAILABLE}>>:-framework Security>
        $<$<AND:$<BOOL:${REACTORMQ_PLATFORM_DARWIN_FAMILY}>,$<BOOL:${REACTORMQ_OPENSSL_AVAILABLE}>>:-framework CoreFoundation>
)

if (REACTORMQ_WITH_THREADS)
    find_package(Threads REQUIRED)
    target_link_libraries(reactormq PRIVATE Threads::Threads)
endif ()

if (REACTORMQ_OPENSSL_AVAILABLE)
    if (TARGET OpenSSL::SSL AND TARGET OpenSSL::Crypto)
        target_link_libraries(reactormq PRIVATE OpenSSL::SSL OpenSSL::Crypto)
        if (WIN32 AND NOT REACTORMQ_OPENSSL_USE_SHARED_LIBS)
            target_link_libraries(reactormq PRIVATE crypt32 bcrypt)
        endif ()
        if (UNIX AND NOT APPLE AND NOT REACTORMQ_OPENSSL_USE_SHARED_LIBS)
            find_library(LIBDL dl)
            if (LIBDL)
                target_link_libraries(reactormq PRIVATE ${LIBDL})
            endif ()
        endif ()
        message(STATUS "Linked OpenSSL to reactormq library")
    else ()
        message(WARNING "REACTORMQ_OPENSSL_AVAILABLE is TRUE but OpenSSL targets not found")
    endif ()
endif ()

# --------------------------------------------------------------------------------------
# Compile flags
if (MSVC)
    target_compile_options(reactormq PRIVATE /permissive- /Zc:__cplusplus /Zc:preprocessor)
    if (NOT REACTORMQ_WITH_RTTI)
        target_compile_options(reactormq PRIVATE /GR-)
    endif ()
    if (NOT REACTORMQ_WITH_EXCEPTIONS)
        target_compile_definitions(reactormq PUBLIC _HAS_EXCEPTIONS=0)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            target_compile_options(reactormq PRIVATE -fno-exceptions)
        endif ()
    endif ()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    if (NOT REACTORMQ_WITH_EXCEPTIONS)
        target_compile_options(reactormq PRIVATE -fno-exceptions)
    endif ()
    if (NOT REACTORMQ_WITH_RTTI)
        target_compile_options(reactormq PRIVATE -fno-rtti)
    endif ()
    target_compile_options(reactormq PRIVATE -fvisibility=hidden -fvisibility-inlines-hidden)
endif ()

# --------------------------------------------------------------------------------------
# Tests
if (REACTORMQ_BUILD_TESTS)
    include(CTest)
    enable_testing()

    include(FetchContent)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)

    add_subdirectory(tests)
endif ()