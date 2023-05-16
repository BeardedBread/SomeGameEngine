cmake_minimum_required(VERSION 3.5.1)
project(sc_map C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(
    sc_map STATIC
        sc_map.c
        sc_map.h)

target_include_directories(sc_map PUBLIC ${CMAKE_CURRENT_LIST_DIR})
