set(PROJECT_NAME HATPC_remake)

cmake_minimum_required(VERSION 3.22.1)
project(${PROJECT_NAME} C)
set(CMAKE_C_STANDARD 99)

add_subdirectory(sc)
add_executable(${PROJECT_NAME}
    main.c
    entManager.c
    mempool.c
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(${PROJECT_NAME}
    sc_queue
    sc_map
)
