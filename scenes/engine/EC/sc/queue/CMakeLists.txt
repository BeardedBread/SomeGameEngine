cmake_minimum_required(VERSION 3.5.1)
project(sc_queue C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(
    sc_queue STATIC
        sc_queue.h)

set_target_properties(sc_queue PROPERTIES LINKER_LANGUAGE C)

target_include_directories(sc_queue PUBLIC ${CMAKE_CURRENT_LIST_DIR})
