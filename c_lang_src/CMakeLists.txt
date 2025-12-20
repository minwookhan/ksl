cmake_minimum_required(VERSION 3.15)
project(ksl-cli-cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
find_package(PkgConfig QUIET)

# Try finding OpenCV via CMake Config first, then PkgConfig
find_package(OpenCV QUIET)
if(NOT OpenCV_FOUND)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(OpenCV REQUIRED opencv4)
        link_directories(${OpenCV_LIBRARY_DIRS})
        include_directories(${OpenCV_INCLUDE_DIRS})
    else()
        message(FATAL_ERROR "OpenCV not found and PkgConfig not found. Please install libopencv-dev.")
    endif()
endif()

find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(CURL REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0 gobject-2.0)
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
message(STATUS "Python3 Executable: ${Python3_EXECUTABLE}")

# Find pybind11
# First try standard CONFIG mode
find_package(pybind11 CONFIG)
# If not found, try finding via Python module
if(NOT pybind11_FOUND)
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -m pybind11 --cmakedir
        OUTPUT_VARIABLE pybind11_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE pybind11_RET
    )
    message(STATUS "pybind11 --cmakedir output: ${pybind11_DIR}")
    
    if(pybind11_DIR AND EXISTS ${pybind11_DIR})
        find_package(pybind11 CONFIG REQUIRED)
    else()
        message(FATAL_ERROR "pybind11 not found. Please install: pip install pybind11 (in ${Python3_EXECUTABLE})")
    endif()
endif()

# Protobuf Generation
set(PROTO_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/protos)
set(PROTO_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${PROTO_BINARY_DIR})

set(PROTO_FILES ${PROTO_SRC_DIR}/ksl_sentence_recognition.proto)

# gRPC plugin finding
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)

# Generate Protobuf and gRPC sources
add_custom_command(
    OUTPUT "${PROTO_BINARY_DIR}/ksl_sentence_recognition.pb.cc"
           "${PROTO_BINARY_DIR}/ksl_sentence_recognition.pb.h"
           "${PROTO_BINARY_DIR}/ksl_sentence_recognition.grpc.pb.cc"
           "${PROTO_BINARY_DIR}/ksl_sentence_recognition.grpc.pb.h"
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS --grpc_out="${PROTO_BINARY_DIR}"
         --cpp_out="${PROTO_BINARY_DIR}"
         --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
         -I "${PROTO_SRC_DIR}"
         "${PROTO_FILES}"
    DEPENDS "${PROTO_FILES}"
)

# Source Files
set(SOURCES
    src/main.cpp
    src/Logic.cpp
    src/PoseEstimator.cpp
    src/Network.cpp
    src/Utils.cpp
    ${PROTO_BINARY_DIR}/ksl_sentence_recognition.pb.cc
    ${PROTO_BINARY_DIR}/ksl_sentence_recognition.grpc.pb.cc
)

# Headers
include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${PROTO_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${Python3_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
)

# Executable
add_executable(ksl-cli-cpp ${SOURCES})

# Link Libraries
target_link_libraries(ksl-cli-cpp
    PRIVATE
    ${OpenCV_LIBS}
    gRPC::grpc++
    protobuf::libprotobuf
    pybind11::embed
    Python3::Python
    CURL::libcurl
    ${GLIB_LIBRARIES}
)

# Copy python script to build directory for embedding
add_custom_command(TARGET ksl-cli-cpp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/mp_detect.py"
    "$<TARGET_FILE_DIR:ksl-cli-cpp>/mp_detect.py"
)
