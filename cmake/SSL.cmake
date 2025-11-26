include_guard(GLOBAL)

# Section: Options

set(REACTORMQ_SSL_PROVIDER "system" CACHE STRING "SSL provider: system, libressl, awslc")
set_property(CACHE REACTORMQ_SSL_PROVIDER PROPERTY STRINGS system libressl awslc)

option(REACTORMQ_OPENSSL_USE_SHARED_LIBS "Link OpenSSL as shared libraries" ON)

# Section: Configure SSL

function(reactormq_configure_ssl)
    set(REACTORMQ_SSL_AVAILABLE FALSE)

    if (REACTORMQ_ENABLE_UE5)
        message(STATUS "UE5 context detected - OpenSSL handled by Unreal Build Tool")
        set(REACTORMQ_SSL_AVAILABLE TRUE)
        set(REACTORMQ_WITH_UE5 ON PARENT_SCOPE)

    elseif (COMMAND ly_add_target)
        message(STATUS "O3DE Gem context - OpenSSL will be provided by O3DE dependencies")
        set(REACTORMQ_SSL_AVAILABLE TRUE)
        set(REACTORMQ_WITH_O3DE ON PARENT_SCOPE)

    elseif (REACTORMQ_SSL_PROVIDER STREQUAL "libressl")
        include(FetchContent)

        FetchContent_Declare(
                libressl
                URL https://ftp.openbsd.org/pub/OpenBSD/LibreSSL/libressl-4.2.1.tar.gz
                URL_HASH SHA256=6d5c2f58583588ea791f4c8645004071d00dfa554a5bf788a006ca1eb5abd70b
                DOWNLOAD_EXTRACT_TIMESTAMP true
        )

        set(ENABLE_EXTRAS OFF CACHE BOOL "" FORCE)
        set(ENABLE_TESTS OFF CACHE BOOL "" FORCE)
        set(LIBRESSL_TESTS OFF CACHE BOOL "" FORCE)
        set(LIBRESSL_APPS OFF CACHE BOOL "" FORCE)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

        FetchContent_MakeAvailable(libressl)
        add_library(OpenSSL::Crypto ALIAS crypto)
        add_library(OpenSSL::SSL ALIAS ssl)
        add_library(LibreSSL::TLS ALIAS tls)

        set(REACTORMQ_SSL_AVAILABLE TRUE)

    elseif (REACTORMQ_SSL_PROVIDER STREQUAL "awslc")
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

        add_library(OpenSSL::Crypto ALIAS crypto)
        add_library(OpenSSL::SSL ALIAS ssl)

        include(CheckIncludeFile)

        if (MSVC)
            if (TARGET crypto)
                target_compile_options(crypto PRIVATE /wd5262)
            endif ()
            if (TARGET ssl)
                target_compile_options(ssl PRIVATE /wd5262)
            endif ()
        endif ()

        set(REACTORMQ_SSL_AVAILABLE TRUE)

    elseif (REACTORMQ_SSL_PROVIDER STREQUAL "system")
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
            set(REACTORMQ_SSL_AVAILABLE TRUE)
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

    set_property(GLOBAL PROPERTY REACTORMQ_SSL_AVAILABLE ${REACTORMQ_SSL_AVAILABLE})

    if (REACTORMQ_SSL_AVAILABLE)
        message(STATUS "OpenSSL integration: ENABLED")

        if (REACTORMQ_WITH_SOCKET_POLYFILL OR REACTORMQ_ENABLE_UE5 OR REACTORMQ_WITH_O3DE)
            set_property(GLOBAL PROPERTY REACTORMQ_SECURE_SOCKET_WITH_BIO_TLS 1)
            set_property(GLOBAL PROPERTY REACTORMQ_SECURE_SOCKET_WITH_TLS 0)
            message(STATUS "Secure socket implementation: BIO (for polyfill/UE5/O3DE platforms)")
        else ()
            set_property(GLOBAL PROPERTY REACTORMQ_SECURE_SOCKET_WITH_TLS 1)
            set_property(GLOBAL PROPERTY REACTORMQ_SECURE_SOCKET_WITH_BIO_TLS 0)
            message(STATUS "Secure socket implementation: Native (using socket descriptor)")
        endif ()
    else ()
        message(STATUS "OpenSSL integration: DISABLED (native socket TLS features will be unavailable)")
        set_property(GLOBAL PROPERTY REACTORMQ_SECURE_SOCKET_WITH_TLS 0)
        set_property(GLOBAL PROPERTY REACTORMQ_SECURE_SOCKET_WITH_BIO_TLS 0)
    endif ()
endfunction()

# Section: Target SSL

function(reactormq_target_ssl target)
    if (NOT TARGET ${target})
        message(FATAL_ERROR "reactormq_target_ssl: target '${target}' does not exist")
    endif ()

    get_property(_ssl_available GLOBAL PROPERTY REACTORMQ_SSL_AVAILABLE)
    get_property(_with_tls GLOBAL PROPERTY REACTORMQ_SECURE_SOCKET_WITH_TLS)
    get_property(_with_bio_tls GLOBAL PROPERTY REACTORMQ_SECURE_SOCKET_WITH_BIO_TLS)

    target_compile_definitions(${target} PRIVATE
            REACTORMQ_WITH_TLS=$<BOOL:${_ssl_available}>
            REACTORMQ_SECURE_SOCKET_WITH_TLS=${_with_tls}
            REACTORMQ_SECURE_SOCKET_WITH_BIO_TLS=${_with_bio_tls}
    )

    if (_ssl_available AND TARGET ssl AND TARGET crypto)
        target_link_libraries(${target} PRIVATE ssl crypto)

        if (WIN32 AND NOT REACTORMQ_OPENSSL_USE_SHARED_LIBS)
            target_link_libraries(${target} PRIVATE crypt32 bcrypt)
        endif ()

        if (UNIX AND NOT APPLE AND NOT REACTORMQ_OPENSSL_USE_SHARED_LIBS)
            find_library(LIBDL dl)
            if (LIBDL)
                target_link_libraries(${target} PRIVATE ${LIBDL})
            endif ()
        endif ()
    endif ()
endfunction()
