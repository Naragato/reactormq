include_guard(GLOBAL)

# POSIX Family Configuration

function(reactormq_family_configure_posix target)
    if (NOT TARGET ${target})
        return()
    endif ()

    message(STATUS "setup posix family")

    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_POSIX_SOCKET 1)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_WINSOCKET 0)

    if (REACTORMQ_WITH_THREADS)
        find_package(Threads REQUIRED)
        target_link_libraries(${target} PRIVATE Threads::Threads)
    endif ()
    set_property(GLOBAL PROPERTY REACTORMQ_PLATFORM_POSIX_FAMILY 1)
endfunction()
