add_subdirectory(sc)
add_library(lib_EC STATIC
    mempool.c
    entManager.c
)
target_include_directories(lib_EC
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${RAYLIB_DIR}/include
)
target_link_libraries(lib_EC
    PRIVATE
    raylib
)
