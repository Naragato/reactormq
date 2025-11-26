include_guard(GLOBAL)

# PlayStation Family Configuration
include("${CMAKE_CURRENT_LIST_DIR}/PosixFamily.cmake")

function(reactormq_family_configure_playstation target)
    if (NOT TARGET ${target})
        return()
    endif ()
    message(STATUS "setup playstation family ${target}")

    reactormq_family_configure_posix(${target})

    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_POSIX_SOCKET 0)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_SONY_SOCKET 1)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_GETHOSTNAME 0)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_GETADDRINFO 0)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_GETNAMEINFO 0)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_SELECT 0)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_IOCTL 0)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_MSG_DONTWAIT 1)
    set_property(GLOBAL PROPERTY REACTORMQ_PLATFORM_SONY_FAMILY 1)
endfunction()
