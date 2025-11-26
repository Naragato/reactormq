include_guard(GLOBAL)

# O3DE Engine Configuration
# O3DE provides its own networking layer, so we disable native platform sockets

function(reactormq_engine_configure target)
    if (NOT TARGET ${target})
        return()
    endif ()

    # O3DE uses its own networking layer (AzNetworking)
    # Disable all native platform socket implementations
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_WINSOCKET 0)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_POSIX_SOCKET 0)
    set_property(GLOBAL PROPERTY REACTORMQ_SOCKET_WITH_SONY_SOCKET 0)

    set(REACTORMQ_WITH_O3DE ON CACHE BOOL "Building as O3DE Gem" FORCE)
endfunction()
