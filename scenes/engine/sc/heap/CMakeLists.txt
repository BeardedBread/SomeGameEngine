cmake_minimum_required(VERSION 3.5.1)
project(sc_heap C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(
    sc_heap STATIC
        sc_heap.c
        sc_heap.h)

target_include_directories(sc_heap PUBLIC ${CMAKE_CURRENT_LIST_DIR})
