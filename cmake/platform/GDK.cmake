include_guard(GLOBAL)

# GDK (Xbox) Platform Configuration

include("${CMAKE_CURRENT_LIST_DIR}/Families/ConsoleFamily.cmake")

function(reactormq_platform_configure target)
    if (NOT TARGET ${target})
        return()
    endif ()

    message(STATUS "setup gdk")

    reactormq_console_configure(${target})
    set_property(GLOBAL PROPERTY REACTORMQ_PLATFORM_GDK 1)
endfunction()
