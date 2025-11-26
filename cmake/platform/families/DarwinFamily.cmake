include_guard(GLOBAL)

# Darwin Family Configuration

include("${CMAKE_CURRENT_LIST_DIR}/PosixFamily.cmake")

function(reactormq_family_configure_darwin target)
    if (NOT TARGET ${target})
        return()
    endif ()

    message(STATUS "setup darwin family")

    reactormq_family_configure_posix(${target})

    set_property(GLOBAL PROPERTY REACTORMQ_PLATFORM_HAS_BSD_TIME 1)
    set_property(GLOBAL PROPERTY REACTORMQ_PLATFORM_HAS_BSD_IPV6_SOCKETS 1)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_MSG_DONTWAIT 1)

    find_library(SECURITY_FRAMEWORK Security)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)

    if (SECURITY_FRAMEWORK)
        target_link_libraries(${target} PUBLIC ${SECURITY_FRAMEWORK})
        target_link_options(${target} PRIVATE -framework Security)
    endif ()

    if (COREFOUNDATION_FRAMEWORK)
        target_link_libraries(${target} PUBLIC ${COREFOUNDATION_FRAMEWORK})
        target_link_options(${target} PRIVATE -framework CoreFoundation)
    endif ()
    set_property(GLOBAL PROPERTY REACTORMQ_PLATFORM_DARWIN_FAMILY 1)
endfunction()
