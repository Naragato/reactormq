include_guard(GLOBAL)

# Console Family Overlay

function(reactormq_console_configure target)
    if (NOT TARGET ${target})
        return()
    endif ()

    message(STATUS "setup console family")

    set(REACTORMQ_BUILD_TESTS OFF CACHE BOOL "Tests disabled for console platforms" FORCE)
    set_property(GLOBAL PROPERTY REACTORMQ_PLATFORM_CONSOLE_FAMILY 1)
endfunction()
