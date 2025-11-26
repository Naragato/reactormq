include_guard(GLOBAL)

# Android Platform Configuration

function(reactormq_platform_configure target)
    if (NOT TARGET ${target})
        return()
    endif ()

    message(STATUS "setup android")

    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_IOCTL 1)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_MSG_DONTWAIT 1)
    set_property(GLOBAL PROPERTY REACTORMQ_PLATFORM_ANDROID 1)
endfunction()
