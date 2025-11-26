include_guard(GLOBAL)

# Platform Configuration System

# Section: Project Defaults

function(reactormq_set_defaults)
    cmake_policy(SET CMP0091 NEW)

    set(CMAKE_CXX_STANDARD 20 PARENT_SCOPE)
    set(CMAKE_CXX_STANDARD_REQUIRED ON PARENT_SCOPE)
    set(CMAKE_CXX_EXTENSIONS OFF PARENT_SCOPE)
    option(REACTORMQ_BUILD_SHARED "Build shared library" OFF)
    option(REACTORMQ_BUILD_TESTS "Build tests" ON)
    option(REACTORMQ_WITH_THREADS "Enable threading support" ON)
    option(REACTORMQ_WITH_EXCEPTIONS "Enable C++ exceptions" OFF)
    option(REACTORMQ_WITH_RTTI "Enable RTTI (typeid/dynamic_cast)" OFF)
    option(REACTORMQ_WITH_CONSOLE_SINK "Enable built-in console log sink" ON)
    option(REACTORMQ_WITH_FILE_SINK "Enable built-in file log sink" ON)
    option(REACTORMQ_ENABLE_UE5 "Enable UE5 integration" OFF)

    set(REACTORMQ_SSL_PROVIDER "libressl" CACHE STRING "TLS provider (system|libressl|awslc|none)")
    set_property(CACHE REACTORMQ_SSL_PROVIDER PROPERTY STRINGS system awslc libressl none)
    option(REACTORMQ_OPENSSL_USE_SHARED_LIBS "Use shared OpenSSL libraries" OFF)

    option(REACTORMQ_WITH_SOCKET_POLYFILL "Enable BSD socket polyfill header" OFF)

    option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
    option(ENABLE_MSAN "Enable MemorySanitizer" OFF)
    option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_POSIX_SOCKET 1)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_WINSOCKET 0)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_SONY_SOCKET 0)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_IOCTL 1)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_POLL 0)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_SELECT 1)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_GETHOSTNAME 1)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_GETADDRINFO 1)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_GETNAMEINFO 1)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_CLOSE_ON_EXEC 0)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_MSG_DONTWAIT 0)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_RECVMMSG 0)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_TIMESTAMP 0)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_NODELAY 1)
    set_property(GLOBAL PROPERTY REACTORMQ_PLATFORM_HAS_BSD_TIME 0)
    set_property(GLOBAL PROPERTY REACTORMQ_PLATFORM_HAS_BSD_IPV6_SOCKETS 0)

    set_property(GLOBAL PROPERTY REACTORMQ_SECURE_SOCKET_WITH_TLS 0)
    set_property(GLOBAL PROPERTY REACTORMQ_SECURE_SOCKET_WITH_BIO_TLS 0)
endfunction()

# Section: Platform Detection

function(reactormq_detect_platform OUT_FAMILY OUT_PLATFORM)
    set(_family "")
    set(_platform "")
    set(_is_console FALSE)

    if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(_family "WindowsFamily")
        set(_platform "Windows")
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(_family "PosixFamily")
        set(_platform "Linux")
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        string(TOLOWER "${CMAKE_OSX_SYSROOT}" _osx_sysroot_lower)
        if (NOT CMAKE_OSX_SYSROOT OR _osx_sysroot_lower MATCHES "macosx")
            set(_family "DarwinFamily")
            set(_platform "MacOS")
        elseif (_osx_sysroot_lower MATCHES "iphoneos")
            set(_family "DarwinFamily")
            set(_platform "iOS")
        else ()
            set(_family "DarwinFamily")
            set(_platform "MacOS")
        endif ()
    elseif (CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(_family "DarwinFamily")
        set(_platform "iOS")
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Android")
        set(_family "PosixFamily")
        set(_platform "Android")
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Prospero")
        set(_family "PlaystationFamily")
        set(_platform "Prospero")
        set(_is_console TRUE)
    elseif (CMAKE_SYSTEM_NAME MATCHES "Gaming\\.Xbox\\.Scarlett\\.x64")
        set(_family "WindowsFamily")
        set(_platform "GDK")
        set(_is_console TRUE)
    elseif (CMAKE_SYSTEM_NAME STREQUAL "NX")
        set(_family "NintendoFamily")
        set(_platform "Switch")
        set(_is_console TRUE)
    else ()
        message(WARNING "Unrecognized platform: ${CMAKE_SYSTEM_NAME}. Using PosixFamily defaults.")
        set(_family "PosixFamily")
        set(_platform "Linux")
    endif ()

    set(${OUT_FAMILY} ${_family} PARENT_SCOPE)
    set(${OUT_PLATFORM} ${_platform} PARENT_SCOPE)
    set(REACTORMQ_IS_CONSOLE ${_is_console} PARENT_SCOPE)

    message(STATUS "Detected platform: ${_platform} (family: ${_family})")
    if (_is_console)
        message(STATUS "  Console platform detected")
    endif ()
endfunction()

# Section: Family and Platform Application

function(reactormq_apply_family target family)
    set(_family_file "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/platform/Families/${family}.cmake")
    string(TOLOWER "${family}" family_lc)
    string(REGEX REPLACE "family$" "" family_lc "${family_lc}")
    set(_fn "reactormq_family_configure_${family_lc}")
    if (EXISTS "${_family_file}")
        include("${_family_file}")
        if (COMMAND "${_fn}")
            cmake_language(CALL "${_fn}" "${target}")
        endif ()
    else ()
        message(WARNING "Family file not found: ${_family_file}")
    endif ()
endfunction()

function(reactormq_apply_platform target platform)
    set(_platform_file "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/platform/${platform}.cmake")
    if (EXISTS "${_platform_file}")
        include("${_platform_file}")
        if (COMMAND reactormq_platform_configure)
            reactormq_platform_configure(${target})
        endif ()
    else ()
        message(WARNING "Platform file not found: ${_platform_file}")
    endif ()
endfunction()

# Section: Target Configuration Helpers

function(reactormq_target_warnings target)
    if (NOT TARGET ${target})
        message(FATAL_ERROR "reactormq_target_warnings: target '${target}' does not exist")
    endif ()

    if (MSVC)
        target_compile_options(${target} PRIVATE
                /W4
                /permissive-
                /Zc:__cplusplus
                /Zc:preprocessor
        )
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target} PRIVATE
                -Wall
                -Wextra
                -Wpedantic
        )
    endif ()
endfunction()

function(reactormq_target_features target)
    if (NOT TARGET ${target})
        message(FATAL_ERROR "reactormq_target_features: target '${target}' does not exist")
    endif ()

    if (MSVC)
        if (NOT REACTORMQ_WITH_RTTI)
            target_compile_options(${target} PRIVATE /GR-)
        endif ()
        if (NOT REACTORMQ_WITH_EXCEPTIONS)
            target_compile_definitions(${target} PUBLIC _HAS_EXCEPTIONS=0)
            if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
                target_compile_options(${target} PRIVATE -fno-exceptions)
            endif ()
        endif ()
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        if (NOT REACTORMQ_WITH_EXCEPTIONS)
            target_compile_options(${target} PRIVATE -fno-exceptions)
        endif ()
        if (NOT REACTORMQ_WITH_RTTI)
            target_compile_options(${target} PRIVATE -fno-rtti)
        endif ()
        target_compile_options(${target} PRIVATE
                -fvisibility=hidden
                -fvisibility-inlines-hidden
        )
    endif ()
endfunction()

function(reactormq_add_sanitizers target)
    if (NOT TARGET ${target})
        message(FATAL_ERROR "reactormq_add_sanitizers: target '${target}' does not exist")
    endif ()

    if (WIN32)
        if (ENABLE_MSAN)
            message(WARNING "MemorySanitizer is not supported on Windows; ignoring.")
            set(ENABLE_MSAN OFF CACHE BOOL "" FORCE)
        endif ()
        if (ENABLE_TSAN)
            message(WARNING "ThreadSanitizer is not supported on Windows; ignoring.")
            set(ENABLE_TSAN OFF CACHE BOOL "" FORCE)
        endif ()
    endif ()

    set(_san_flags "")
    set(_san_link_flags "")

    if (ENABLE_ASAN)
        if (MSVC)
            list(APPEND _san_flags /fsanitize=address)
            list(APPEND _san_link_flags /fsanitize=address)
        else ()
            list(APPEND _san_flags -fsanitize=address)
            list(APPEND _san_link_flags -fsanitize=address)
        endif ()
    endif ()

    if (ENABLE_MSAN AND NOT WIN32)
        list(APPEND _san_flags
                -fsanitize=memory
                -fsanitize-memory-track-origins=2
                -fno-omit-frame-pointer
        )
        list(APPEND _san_link_flags
                -fsanitize=memory
                -fsanitize-memory-track-origins=2
        )
    endif ()

    if (ENABLE_TSAN AND NOT WIN32)
        list(APPEND _san_flags -fsanitize=thread)
        list(APPEND _san_link_flags -fsanitize=thread)
    endif ()

    if (_san_flags)
        target_compile_options(${target} PRIVATE ${_san_flags})
    endif ()
    if (_san_link_flags)
        target_link_options(${target} PRIVATE ${_san_link_flags})
    endif ()

    if (ENABLE_ASAN OR ENABLE_MSAN OR ENABLE_TSAN)
        target_compile_definitions(${target} PRIVATE REACTORMQ_WITH_SANITIZERS=1)
    endif ()
endfunction()

function(reactormq_disable_msan_for_targets)
    if (NOT ENABLE_MSAN)
        return()
    endif ()

    foreach (tgt IN LISTS ARGN)
        if (TARGET ${tgt})
            if (NOT WIN32)
                target_compile_options(${tgt} PRIVATE -fno-sanitize=memory)
                target_link_options(${tgt} PRIVATE -fno-sanitize=memory)
            endif ()
        endif ()
    endforeach ()
endfunction()

function(reactormq_collect_socket_sources OUT_VAR sources_dir)
    set(_socket_sources "")

    get_property(_with_posix GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_POSIX_SOCKET)
    get_property(_with_winsocket GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_WINSOCKET)
    get_property(_with_sony GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_SONY_SOCKET)
    get_property(_with_tls GLOBAL PROPERTY REACTORMQ_SECURE_SOCKET_WITH_TLS)
    get_property(_with_bio_tls GLOBAL PROPERTY REACTORMQ_SECURE_SOCKET_WITH_BIO_TLS)

    if (_with_winsocket)
        list(APPEND _socket_sources "${sources_dir}/socket/platform/win_socket.cpp")
    endif ()
    if (_with_posix)
        list(APPEND _socket_sources "${sources_dir}/socket/platform/posix_socket.cpp")
    endif ()
    if (_with_sony)
        list(APPEND _socket_sources "${sources_dir}/socket/platform/sony_socket.cpp")
    endif ()
    if (_with_tls)
        list(APPEND _socket_sources "${sources_dir}/socket/platform/native_secure_socket.cpp")
    endif ()
    if (_with_bio_tls)
        list(APPEND _socket_sources "${sources_dir}/socket/platform/bio_secure_socket.cpp")
    endif ()

    set(${OUT_VAR} ${_socket_sources} PARENT_SCOPE)
endfunction()

function(reactormq_target_sockets target)
    if (NOT TARGET ${target})
        message(FATAL_ERROR "reactormq_target_sockets: target '${target}' does not exist")
    endif ()

    get_property(_with_posix GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_POSIX_SOCKET)
    get_property(_with_winsocket GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_WINSOCKET)
    get_property(_with_sony GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_SONY_SOCKET)
    get_property(_with_ioctl GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_IOCTL)
    get_property(_with_poll GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_POLL)
    get_property(_with_select GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_SELECT)
    get_property(_with_gethostname GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_GETHOSTNAME)
    get_property(_with_getaddrinfo GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_GETADDRINFO)
    get_property(_with_getnameinfo GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_GETNAMEINFO)
    get_property(_with_close_on_exec GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_CLOSE_ON_EXEC)
    get_property(_with_msg_dontwait GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_MSG_DONTWAIT)
    get_property(_with_recvmmsg GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_RECVMMSG)
    get_property(_with_timestamp GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_TIMESTAMP)
    get_property(_with_nodelay GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_NODELAY)
    get_property(_has_bsd_time GLOBAL PROPERTY REACTORMQ_PLATFORM_HAS_BSD_TIME)
    get_property(_has_bsd_ipv6 GLOBAL PROPERTY REACTORMQ_PLATFORM_HAS_BSD_IPV6_SOCKETS)

    get_property(_with_posix GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_POSIX_SOCKET)
    get_property(_with_winsocket GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_WINSOCKET)
    get_property(_with_sony GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_SONY_SOCKET)
    get_property(_with_tls GLOBAL PROPERTY REACTORMQ_SECURE_SOCKET_WITH_TLS)
    get_property(_with_bio_tls GLOBAL PROPERTY REACTORMQ_SECURE_SOCKET_WITH_BIO_TLS)

    get_property(_platform_windows GLOBAL PROPERTY REACTORMQ_PLATFORM_WINDOWS)
    get_property(_platform_linux GLOBAL PROPERTY REACTORMQ_PLATFORM_LINUX)
    get_property(_platform_android GLOBAL PROPERTY REACTORMQ_PLATFORM_ANDROID)
    get_property(_platform_macos GLOBAL PROPERTY REACTORMQ_PLATFORM_MACOS)
    get_property(_platform_ios GLOBAL PROPERTY REACTORMQ_PLATFORM_IOS)
    get_property(_platform_prospero GLOBAL PROPERTY REACTORMQ_PLATFORM_PROSPERO)
    get_property(_platform_nnx GLOBAL PROPERTY REACTORMQ_PLATFORM_NNX)
    get_property(_family_windows GLOBAL PROPERTY REACTORMQ_PLATFORM_WINDOWS_FAMILY)
    get_property(_family_darwin GLOBAL PROPERTY REACTORMQ_PLATFORM_DARWIN_FAMILY)
    get_property(_family_posix GLOBAL PROPERTY REACTORMQ_PLATFORM_POSIX_FAMILY)
    get_property(_family_console GLOBAL PROPERTY REACTORMQ_PLATFORM_CONSOLE_FAMILY)
    get_property(_family_nintendo GLOBAL PROPERTY REACTORMQ_PLATFORM_NINTENDO_FAMILY)


    target_compile_definitions(${target} PRIVATE
            REACTORMQ_SOCKET_WITH_POSIX_SOCKET=${_with_posix}
            REACTORMQ_SOCKET_WITH_WINSOCKET=${_with_winsocket}
            REACTORMQ_SOCKET_WITH_SONY_SOCKET=${_with_sony}
            REACTORMQ_SOCKET_WITH_IOCTL=${_with_ioctl}
            REACTORMQ_SOCKET_WITH_POLL=${_with_poll}
            REACTORMQ_SOCKET_WITH_SELECT=${_with_select}
            REACTORMQ_SOCKET_WITH_GETHOSTNAME=${_with_gethostname}
            REACTORMQ_SOCKET_WITH_GETADDRINFO=${_with_getaddrinfo}
            REACTORMQ_SOCKET_WITH_GETNAMEINFO=${_with_getnameinfo}
            REACTORMQ_SOCKET_WITH_CLOSE_ON_EXEC=${_with_close_on_exec}
            REACTORMQ_SOCKET_WITH_MSG_DONTWAIT=${_with_msg_dontwait}
            REACTORMQ_SOCKET_WITH_RECVMMSG=${_with_recvmmsg}
            REACTORMQ_SOCKET_WITH_TIMESTAMP=${_with_timestamp}
            REACTORMQ_SOCKET_WITH_NODELAY=${_with_nodelay}
            REACTORMQ_PLATFORM_HAS_BSD_TIME=${_has_bsd_time}
            REACTORMQ_PLATFORM_HAS_BSD_IPV6_SOCKETS=${_has_bsd_ipv6}
            REACTORMQ_WITH_SOCKET_POLYFILL=$<BOOL:${REACTORMQ_WITH_SOCKET_POLYFILL}>

            REACTORMQ_PLATFORM_WINDOWS=$<BOOL:${_platform_windows}>
            REACTORMQ_PLATFORM_LINUX=$<BOOL:${_platform_linux}>
            REACTORMQ_PLATFORM_ANDROID=$<BOOL:${_platform_android}>
            REACTORMQ_PLATFORM_MACOS=$<BOOL:${_platform_macos}>
            REACTORMQ_PLATFORM_IOS=$<BOOL:${_platform_ios}>
            REACTORMQ_PLATFORM_PROSPERO=$<BOOL:${_platform_prospero}>
            REACTORMQ_PLATFORM_NNX=$<BOOL:${_platform_nnx}>

            REACTORMQ_PLATFORM_WINDOWS_FAMILY=$<BOOL:${_family_windows}>
            REACTORMQ_PLATFORM_DARWIN_FAMILY=$<BOOL:${_family_darwin}>
            REACTORMQ_PLATFORM_POSIX_FAMILY=$<BOOL:${_family_posix}>
            REACTORMQ_PLATFORM_CONSOLE_FAMILY=$<BOOL:${_family_console}>
            REACTORMQ_PLATFORM_NINTENDO_FAMILY=$<BOOL:${_family_nintendo}>
    )

    if (_with_winsocket)
        target_link_libraries(${target} PRIVATE ws2_32)
    endif ()
endfunction()


function(reactormq_target_install target)
    if (NOT TARGET ${target})
        message(FATAL_ERROR "reactormq_target_install: target '${target}' does not exist")
    endif ()

    install(TARGETS ${target}
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
    )
endfunction()

function(reactormq_configure_tests_for_platform)
    if (REACTORMQ_IS_CONSOLE)
        set(REACTORMQ_BUILD_TESTS OFF CACHE BOOL "Tests disabled for console platforms" FORCE)
        message(STATUS "Tests disabled for console platform")
    else ()
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
    endif ()
endfunction()
