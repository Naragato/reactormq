
set(_RMQ_PROSPERO_ROOT "${CMAKE_SOURCE_DIR}")
set(_RMQ_PROSPERO_SHIM "${_RMQ_PROSPERO_ROOT}/shim/prospero")
message(STATUS "Add ${_RMQ_PROSPERO_SHIM}")

include_directories(SYSTEM ${_RMQ_PROSPERO_SHIM})

set(CMAKE_C_FLAGS_INIT
        "${CMAKE_C_FLAGS_INIT} -isystem \"${_RMQ_PROSPERO_SHIM}\"")
set(CMAKE_CXX_FLAGS_INIT
        "${CMAKE_CXX_FLAGS_INIT} -isystem \"${_RMQ_PROSPERO_SHIM}\"")

message(STATUS "C std include dirs: ${CMAKE_C_STANDARD_INCLUDE_DIRECTORIES}")
