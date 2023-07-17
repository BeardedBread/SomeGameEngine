add_subdirectory(EC)
add_library(lib_engine STATIC
    assets.c
    AABB.c
    gui.c
    engine.c
    collisions.c
)
target_include_directories(lib_engine
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(lib_engine
    PUBLIC
    lib_EC
)
