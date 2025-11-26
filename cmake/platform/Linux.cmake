include_guard(GLOBAL)

# Linux Platform Configuration

function(reactormq_platform_configure target)
    if (NOT TARGET ${target})
        return()
    endif ()

    message(STATUS "setup linux")

    set_property(GLOBAL PROPERTY REACTORMQ_PLATFORM_HAS_BSD_IPV6_SOCKETS 1)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_IOCTL 1)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_POLL 1)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_MSG_DONTWAIT 1)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_RECVMMSG 1)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_TIMESTAMP 1)
    set_property(GLOBAL PROPERTY REACTORMQ_PLATFORM_LINUX 1)
endfunction()
