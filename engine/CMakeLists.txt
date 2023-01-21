add_subdirectory(EC)
add_library(lib_scenes STATIC
    scene.c
    scene_impl.c
    game_systems.c
)
target_include_directories(lib_scenes
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${RAYLIB_DIR}/include
)
target_link_libraries(lib_scenes
    PRIVATE
    lib_EC
    raylib
)
