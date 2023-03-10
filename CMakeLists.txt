set(PROJECT_NAME HATPC_remake)
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_FLAGS "-Wall")
cmake_minimum_required(VERSION 3.22.1)
project(${PROJECT_NAME} C)
set(CMAKE_C_STANDARD 99)
set(RAYLIB_DIR $ENV{HOME}/Documents/Coding/raylib/out/)

if (${CMAKE_BUILD_TYPE} STREQUAL tile16)
    set_directory_properties(PROPERTIES COMPILE_DEFINITIONS TILE16_SIZE)
endif()

set(GAME_LIBS
    lib_EC
    lib_scenes
    sc_queue
    sc_map
    sc_array
    raylib
    m
)


add_subdirectory(engine)
add_executable(${PROJECT_NAME}
    main.c
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${RAYLIB_DIR}/include
)
target_link_directories(${PROJECT_NAME}
    PRIVATE
    ${RAYLIB_DIR}/lib
)

target_link_libraries(${PROJECT_NAME}
    ${GAME_LIBS}
)

add_executable(EntManager_test
    entManager_test.c
)
target_compile_options(EntManager_test PRIVATE -fsanitize=address -gdwarf-4)
target_link_options(EntManager_test PRIVATE -fsanitize=address -gdwarf-4)
target_include_directories(EntManager_test
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${RAYLIB_DIR}/include
)
target_link_directories(EntManager_test
    PRIVATE
    ${RAYLIB_DIR}/lib
)
target_link_libraries(EntManager_test
    lib_EC
    lib_scenes
    sc_queue
    sc_map
)

add_executable(scene_test
    scene_test.c
)

target_include_directories(scene_test
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${RAYLIB_DIR}/include
)

target_compile_options(scene_test PRIVATE -fsanitize=address -gdwarf-4)
target_link_options(scene_test PRIVATE -fsanitize=address -gdwarf-4)

target_link_directories(scene_test
    PRIVATE
    ${RAYLIB_DIR}/lib
)

target_link_libraries(scene_test
    ${GAME_LIBS}
)

add_executable(scene_test_mem
    scene_test.c
)
target_include_directories(scene_test_mem
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${RAYLIB_DIR}/include
)
target_link_options(scene_test_mem PRIVATE -Xlinker -Map=scene_test.map)
target_link_directories(scene_test_mem
    PRIVATE
    ${RAYLIB_DIR}/lib
)
target_link_libraries(scene_test_mem
    ${GAME_LIBS}
)

add_executable(menu_test
    menu_test.c
)
target_include_directories(menu_test
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${RAYLIB_DIR}/include
)
target_compile_options(menu_test PRIVATE -fsanitize=address -gdwarf-4)
target_link_options(menu_test PRIVATE -fsanitize=address -gdwarf-4)
target_link_directories(menu_test
    PRIVATE
    ${RAYLIB_DIR}/lib
)
target_link_libraries(menu_test
    ${GAME_LIBS}
)

