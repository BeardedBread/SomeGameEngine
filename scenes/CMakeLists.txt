add_library(lib_scenes STATIC
    assets_loader.c
    player_ent.c
    items_ent.c
    water_flow.c
    editor_scene.c
    menu_scene.c
    level_select_scene.c
    game_scene.c
    game_systems.c
    scene_systems.c
    camera_systems.c
    engine_impl.c
)
target_include_directories(lib_scenes
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../tracy/public/
)

if (${RUN_PROFILER})
    target_compile_definitions(lib_scenes
        PUBLIC
        TRACY_ENABLE
        TRACY_ON_DEMAND
    )
endif()

target_link_libraries(lib_scenes
    PUBLIC
    lib_engine
)
