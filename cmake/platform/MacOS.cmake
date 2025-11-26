include_guard(GLOBAL)

# macOS Platform Configuration

function(reactormq_platform_configure target)
    if (NOT TARGET ${target})
        return()
    endif ()

    message(STATUS "setup macos")

    set_property(GLOBAL PROPERTY REACTORMQ_PLATFORM_MACOS 1)
endfunction()
