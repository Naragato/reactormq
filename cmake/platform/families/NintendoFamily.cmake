include_guard(GLOBAL)

# Nintendo Family Configuration

include("${CMAKE_CURRENT_LIST_DIR}/PosixFamily.cmake")

function(reactormq_family_configure_nintendo target)
    if (NOT TARGET ${target})
        return()
    endif ()

    message(STATUS "setup nintendo family")


    reactormq_family_configure_posix(${target})

    set(REACTORMQ_WITH_SOCKET_POLYFILL ON CACHE BOOL "Enable socket polyfill for Nintendo" FORCE)
    set_property(GLOBAL PROPERTY REACTORMQ_PLATFORM_NINTENDO_FAMILY 1)
endfunction()
