include_guard(GLOBAL)

# Prospero (PlayStation 5) Platform Configuration

include("${CMAKE_CURRENT_LIST_DIR}/Families/ConsoleFamily.cmake")

function(reactormq_platform_configure target)
    if (NOT TARGET ${target})
        return()
    endif ()

    message(STATUS "setup prospero")

    reactormq_console_configure(${target})

    set(_prospero_shim "${CMAKE_SOURCE_DIR}/shim/prospero")
    if (EXISTS "${_prospero_shim}")
        include_directories(SYSTEM ${_prospero_shim})
        set(CMAKE_C_FLAGS_INIT
                "${CMAKE_C_FLAGS_INIT} -isystem \"${_prospero_shim}\"")
        set(CMAKE_CXX_FLAGS_INIT
                "${CMAKE_CXX_FLAGS_INIT} -isystem \"${_prospero_shim}\"")
    endif ()

#    if (TARGET crypto)
#        message(STATUS "crypto setup")
#        target_compile_definitions(crypto PRIVATE
#                OPENSSL_NO_SOCK
#                NO_SYS_UN_H
#                OPENSSL_NO_DLOPEN
#                NO_SYSLOG
#        )
#    endif ()
#
#    if (TARGET ssl)
#
#        target_compile_definitions(ssl PRIVATE
#                OPENSSL_NO_SOCK
#                NO_SYS_UN_H
#                OPENSSL_NO_DLOPEN
#                NO_SYSLOG
#        )
#    endif ()
#
#    if (TARGET crypto_objects)
#        message(STATUS "ssl")
#        target_compile_definitions(crypto_objects PRIVATE
#                OPENSSL_NO_SOCK
#                NO_SYS_UN_H
#                OPENSSL_NO_DLOPEN
#                NO_SYSLOG
#        )
#    endif ()

    if (DEFINED awslc_SOURCE_DIR)
        set(_aws_lc_console_src "${awslc_SOURCE_DIR}/crypto/console/console.c")
        if (EXISTS "${_aws_lc_console_src}")
            set_source_files_properties("${_aws_lc_console_src}"
                    PROPERTIES HEADER_FILE_ONLY TRUE
            )
        endif ()
    endif ()
    set_property(GLOBAL PROPERTY REACTORMQ_PLATFORM_PROSPERO 1)
endfunction()
