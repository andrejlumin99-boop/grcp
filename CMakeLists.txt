cmake_minimum_required(VERSION 3.15)
project(telemetry_service CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)


get_target_property(PROTOC_PROGRAM protobuf::protoc IMPORTED_LOCATION_RELEASE)
if(NOT PROTOC_PROGRAM)
    get_target_property(PROTOC_PROGRAM protobuf::protoc IMPORTED_LOCATION_DEBUG)
endif()
get_filename_component(PROTO_INCLUDE_DIR "${PROTOC_PROGRAM}" DIRECTORY)
set(PROTO_INCLUDE_DIR "${PROTO_INCLUDE_DIR}/../include")

set(PROTO_SRC "${CMAKE_CURRENT_SOURCE_DIR}/proto/telemetry.proto")
set(PROTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

set(PROTO_SRCS "${PROTO_GEN_DIR}/telemetry.pb.cc")
set(PROTO_HDRS "${PROTO_GEN_DIR}/telemetry.pb.h")
set(GRPC_SRCS "${PROTO_GEN_DIR}/telemetry.grpc.pb.cc")
set(GRPC_HDRS "${PROTO_GEN_DIR}/telemetry.grpc.pb.h")

add_custom_command(
    OUTPUT "${PROTO_SRCS}" "${PROTO_HDRS}" "${GRPC_SRCS}" "${GRPC_HDRS}"
    COMMAND protobuf::protoc
    ARGS --grpc_out=${PROTO_GEN_DIR} 
         --cpp_out=${PROTO_GEN_DIR} 
         -I ${CMAKE_CURRENT_SOURCE_DIR}/proto 
         -I ${PROTO_INCLUDE_DIR}
         --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin> 
         ${PROTO_SRC}
    DEPENDS ${PROTO_SRC}
)

add_executable(telemetry_server
    src/main.cpp
    src/telemetry_service.h
    src/telemetry_service.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
    ${GRPC_SRCS}
    ${GRPC_HDRS}
)

target_include_directories(telemetry_server PRIVATE ${PROTO_GEN_DIR} src)
target_link_libraries(telemetry_server PRIVATE gRPC::grpc++ protobuf::libprotobuf)