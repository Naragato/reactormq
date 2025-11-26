include_guard(GLOBAL)

# iOS Platform Configuration

function(reactormq_platform_configure target)
    if (NOT TARGET ${target})
        return()
    endif ()

    message(STATUS "setup ios")

    set_property(GLOBAL PROPERTY REACTORMQ_PLATFORM_IOS 1)
endfunction()
