include_guard(GLOBAL)

# Engine Configuration System
# Engines (like O3DE, UE5) can layer on top of platforms and override defaults

# Section: Engine Detection

function(reactormq_detect_engine OUT_ENGINE)
    set(_engine "None")

    if (COMMAND ly_add_target)
        set(_engine "O3DE")
    endif ()

    set(${OUT_ENGINE} ${_engine} PARENT_SCOPE)

    if (NOT _engine STREQUAL "None")
        message(STATUS "Detected engine: ${_engine}")
    endif ()
endfunction()

# Section: Engine Application

function(reactormq_apply_engine target engine)
    if (engine STREQUAL "None")
        return()
    endif ()

    set(_engine_file "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/engine/${engine}.cmake")
    if (EXISTS "${_engine_file}")
        include("${_engine_file}")
        if (COMMAND reactormq_engine_configure)
            reactormq_engine_configure(${target})
        endif ()
    else ()
        message(WARNING "Engine file not found: ${_engine_file}")
    endif ()
endfunction()
