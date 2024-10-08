set(PROJECT_NAME HATPC_remake)
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_FLAGS "-Wall -Wextra")
cmake_minimum_required(VERSION 3.22.1)
project(${PROJECT_NAME} C CXX)
set(CMAKE_C_STANDARD 99)
set(RAYLIB_DIR /usr/local/lib CACHE FILEPATH "directory to Raylib")
set(LIBZSTD_DIR /usr/local/lib CACHE FILEPATH "directory to zstd")

if (EMSCRIPTEN)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPLATFORM_WEB")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 -s TOTAL_MEMORY=16777216 -s TOTAL_STACK=1048576 --preload-file ./res ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif ()

if (${CMAKE_BUILD_TYPE} STREQUAL tile16)
    set_directory_properties(PROPERTIES COMPILE_DEFINITIONS TILE16_SIZE)
endif()

set(GAME_LIBS
    lib_scenes
)

add_subdirectory(engine)
add_subdirectory(scenes)
if (NOT EMSCRIPTEN)
add_subdirectory(res)
endif ()

add_executable(${PROJECT_NAME}
    main.c
)

# Use Heaptrack to profile the main application
# Do not compile in ASAN

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${PROJECT_NAME}
    ${GAME_LIBS}
)

add_executable(sandbox_trace
    scene_test.c
    tracy/public/TracyClient.cpp
)
target_compile_definitions(sandbox_trace
    PUBLIC
    TRACY_ENABLE
    TRACY_ON_DEMAND
)
target_include_directories(sandbox_trace
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    PUBLIC
    tracy/public/
)

target_link_libraries(sandbox_trace
    PUBLIC
    lib_scenes_trace
    pthread
    dl
)

add_executable(scene_test
    scene_test.c
)

target_include_directories(scene_test
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)


if (NOT EMSCRIPTEN)
target_compile_options(scene_test PRIVATE -fsanitize=address -gdwarf-4)
target_link_options(scene_test PRIVATE -fsanitize=address -gdwarf-4)
endif ()
target_link_libraries(scene_test
    ${GAME_LIBS}
)


if (NOT EMSCRIPTEN)

add_executable(EntManager_test
    entManager_test.c
)
target_compile_options(EntManager_test PRIVATE -fsanitize=address -gdwarf-4)
target_link_options(EntManager_test PRIVATE -fsanitize=address -gdwarf-4)
target_include_directories(EntManager_test
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(EntManager_test
    ${GAME_LIBS}
)


add_executable(scene_test_mem
    scene_test.c
)
target_include_directories(scene_test_mem
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_options(scene_test_mem PRIVATE -Xlinker -Map=scene_test.map)

target_link_libraries(scene_test_mem
    ${GAME_LIBS}
)

add_executable(water_test
    water_test.c
)

target_include_directories(water_test
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(water_test
    ${GAME_LIBS}
)
target_compile_options(water_test PRIVATE -fsanitize=address -gdwarf-4)
target_link_options(water_test PRIVATE -fsanitize=address -gdwarf-4)

add_executable(level_load_test
    level_load_test.c
)
target_include_directories(level_load_test
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(level_load_test
    ${GAME_LIBS}
)
target_compile_options(level_load_test PRIVATE -fsanitize=address -gdwarf-4)
target_link_options(level_load_test PRIVATE -fsanitize=address -gdwarf-4)

add_executable(menu_test
    menu_test.c
)
target_include_directories(menu_test
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_compile_options(menu_test PRIVATE -fsanitize=address -gdwarf-4)
target_link_options(menu_test PRIVATE -fsanitize=address -gdwarf-4)
target_link_libraries(menu_test
    ${GAME_LIBS}
)

add_executable(assets_test
    assets_test.c
)
target_include_directories(assets_test
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_compile_options(assets_test PRIVATE -fsanitize=address -gdwarf-4)
target_link_options(assets_test PRIVATE -fsanitize=address -gdwarf-4)
target_link_libraries(assets_test
    ${GAME_LIBS}
)

add_executable(particle_test
    particle_test.c
)
target_include_directories(particle_test
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_compile_options(particle_test PRIVATE -fsanitize=address -gdwarf-4)
target_link_options(particle_test PRIVATE -fsanitize=address -gdwarf-4)
target_link_libraries(particle_test
    ${GAME_LIBS}
)

add_executable(scene_man_test
    scene_man_test.c
)
target_include_directories(scene_man_test
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_compile_options(scene_man_test PRIVATE -fsanitize=address -gdwarf-4)
target_link_options(scene_man_test PRIVATE -fsanitize=address -gdwarf-4)
target_link_libraries(scene_man_test
    ${GAME_LIBS}
)
add_executable(level_select_test
    level_select_test.c
)
target_include_directories(level_select_test
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_compile_options(level_select_test PRIVATE -fsanitize=address -gdwarf-4)
target_link_options(level_select_test PRIVATE -fsanitize=address -gdwarf-4)
target_link_libraries(level_select_test
    ${GAME_LIBS}
)

if (BUILD_TESTING)
     find_package(cmocka 1.1.0 REQUIRED)
     add_subdirectory(tests)
endif()
endif()
