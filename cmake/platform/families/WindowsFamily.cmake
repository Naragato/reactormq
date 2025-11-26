include_guard(GLOBAL)

# Windows Family Configuration

function(reactormq_family_configure_windows target)
    if (NOT TARGET ${target})
        return()
    endif ()

    message(STATUS "setup windows family")


    target_compile_definitions(${target} PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            UNICODE
            _UNICODE
    )

    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_WINSOCKET 1)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_POSIX_SOCKET 0)
    set_property(GLOBAL PROPERTY REACTORMQ_PLATFORM_WINDOWS_FAMILY 1)
endfunction()
