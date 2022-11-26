cmake_minimum_required(VERSION 3.5.1)
project(sc_array C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(sc_array STATIC sc_array.h)
set_target_properties(sc_array PROPERTIES LINKER_LANGUAGE C)

target_include_directories(sc_array PUBLIC ${CMAKE_CURRENT_LIST_DIR})

if (NOT CMAKE_C_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -Wextra -pedantic -Werror")
endif ()
