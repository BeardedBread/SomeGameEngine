add_subdirectory(engine)
add_library(lib_scenes STATIC
    assets_maps.c
    editor_scene.c
    menu_scene.c
    game_systems.c
)
target_include_directories(lib_scenes
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(lib_scenes
    PUBLIC
    lib_engine
)
