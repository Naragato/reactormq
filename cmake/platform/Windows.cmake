include_guard(GLOBAL)

# Windows Platform Configuration

function(reactormq_platform_configure target)
    if (NOT TARGET ${target})
        return()
    endif ()

    message(STATUS "setup windows")

    set_property(GLOBAL PROPERTY REACTORMQ_PLATFORM_WINDOWS 1)
endfunction()
