include_guard()

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_MSAN "Enable MemorySanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

set(_SANITIZER_SUPPORTED TRUE)

if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Sanitizers.cmake: non-Clang compiler: sanitizers may be limited.")
endif ()

if (WIN32)
    if (ENABLE_MSAN)
        message(WARNING "MemorySanitizer is not supported on Windows; disabling ENABLE_MSAN.")
        set(ENABLE_MSAN OFF CACHE BOOL "" FORCE)
    endif ()
    if (ENABLE_TSAN)
        message(WARNING "ThreadSanitizer is not supported on Windows; disabling ENABLE_TSAN.")
        set(ENABLE_TSAN OFF CACHE BOOL "" FORCE)
    endif ()
endif ()

set(SANITIZER_CFLAGS "")
set(SANITIZER_LDFLAGS "")

if (ENABLE_ASAN)
    if (MSVC)
        list(APPEND SANITIZER_CFLAGS "/fsanitize=address")
        list(APPEND SANITIZER_LDFLAGS "/fsanitize=address")
    else ()
        list(APPEND SANITIZER_CFLAGS "-fsanitize=address")
        list(APPEND SANITIZER_LDFLAGS "-fsanitize=address")
    endif ()
endif ()

if (ENABLE_MSAN)
    if (WIN32)
        message(WARNING "ENABLE_MSAN requested on Windows but not supported; ignoring.")
    else ()
        list(APPEND SANITIZER_CFLAGS
                "-fsanitize=memory"
                "-fsanitize-memory-track-origins=2"
                "-fno-omit-frame-pointer"
        )
        list(APPEND SANITIZER_LDFLAGS
                "-fsanitize=memory"
                "-fsanitize-memory-track-origins=2"
        )
    endif ()
endif ()

if (ENABLE_TSAN)
    if (WIN32)
        message(WARNING "ENABLE_TSAN requested on Windows but not supported; ignoring.")
    else ()
        list(APPEND SANITIZER_CFLAGS "-fsanitize=thread")
        list(APPEND SANITIZER_LDFLAGS "-fsanitize=thread")
    endif ()
endif ()

string(JOIN " " SANITIZER_CFLAGS "${SANITIZER_CFLAGS}")
string(JOIN " " SANITIZER_LDFLAGS "${SANITIZER_LDFLAGS}")
set(SANITIZER_CFLAGS "${SANITIZER_CFLAGS}" CACHE STRING "Sanitizer compile flags" FORCE)
set(SANITIZER_LDFLAGS "${SANITIZER_LDFLAGS}" CACHE STRING "Sanitizer link flags" FORCE)

message(STATUS "Sanitizers.cmake: SANITIZER_CFLAGS='${SANITIZER_CFLAGS}'")
message(STATUS "Sanitizers.cmake: SANITIZER_LDFLAGS='${SANITIZER_LDFLAGS}'")

function(reactormq_add_sanitizers target)
    if (NOT TARGET ${target})
        message(FATAL_ERROR "reactormq_add_sanitizers: target '${target}' does not exist")
    endif ()

    if (SANITIZER_CFLAGS)
        target_compile_options(${target} PRIVATE ${SANITIZER_CFLAGS})
    endif ()
    if (SANITIZER_LDFLAGS)
        target_link_options(${target} PRIVATE ${SANITIZER_LDFLAGS})
    endif ()

    if (ENABLE_ASAN OR ENABLE_MSAN OR ENABLE_TSAN)
        target_compile_definitions(${target} PRIVATE
                REACTORMQ_WITH_SANITIZERS=1
        )
    endif ()
endfunction()

function(reactormq_disable_msan_for_targets)
    if (NOT ENABLE_MSAN)
        return()
    endif ()

    foreach (tgt IN LISTS ARGN)
        if (TARGET ${tgt})
            if (NOT WIN32)
                target_compile_options(${tgt} PRIVATE -fno-sanitize=memory)
                target_link_options(${tgt} PRIVATE -fno-sanitize=memory)
            endif ()
        endif ()
    endforeach ()
endfunction()
