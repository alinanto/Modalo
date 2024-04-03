cmake_minimum_required(VERSION 3.10)
project(modalo)
include_directories(${CMAKE_SOURCE_DIR}/includes)

add_executable(modalo main.c)
add_library(modbus SHARED
            modbus.c
            modbus-tcp.c
            modbus-rtu.c
            modbus-data.c)
add_library(error-modalo SHARED modaloError.c)
add_library(parse-modalo SHARED modaloParse.c)
add_library(cJSON SHARED cJSON.c)

target_link_libraries(parse-modalo error-modalo cJSON)
target_link_libraries(modalo modbus error-modalo parse-modalo cJSON)
target_link_libraries(modbus ws2_32)

target_compile_definitions(error-modalo PUBLIC MODALO_EXPORT)
target_compile_definitions(parse-modalo PUBLIC MODALO_EXPORT)
target_compile_definitions(modbus PUBLIC DLLBUILD)

install(TARGETS modalo)