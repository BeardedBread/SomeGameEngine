set(PROJECT_NAME HATPC_remake)
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_FLAGS "-Wall -Wextra")
cmake_minimum_required(VERSION 3.22.1)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(${PROJECT_NAME} C CXX)
set(CMAKE_C_STANDARD 99)
set(RAYLIB_DIR /usr/local/lib CACHE FILEPATH "directory to Raylib")
set(LIBZSTD_DIR /usr/local/lib CACHE FILEPATH "directory to zstd")
option(RUN_PROFILER OFF)
option(INCLUDE_ASAN ON)
option(EXPORT_MMAP OFF)
option(BUILD_EXTRAS OFF)

# If you want to use Heaptrack to profile the memory
# Do not compile in ASAN

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

if (${RUN_PROFILER})
    set(GAME_LIBS
        lib_scenes
        pthread
        dl
    )
endif()

add_subdirectory(engine)
add_subdirectory(scenes)
if (NOT EMSCRIPTEN)
add_subdirectory(res)
endif ()

macro(add_target_exe name)
    add_executable(${name}
    ${name}.c
    tracy/public/TracyClient.cpp
    )
    target_include_directories(${name}
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        PUBLIC
        tracy/public/
    )
    if (RUN_PROFILER)
        target_compile_definitions(${name}
            PUBLIC
            TRACY_ENABLE
            TRACY_ON_DEMAND
        )
    endif()

    if (NOT EMSCRIPTEN)
        if (INCLUDE_ASAN)
            target_compile_options(${name} PRIVATE -fsanitize=address -gdwarf-4)
            target_link_options(${name} PRIVATE -fsanitize=address -gdwarf-4)
        endif ()
        if (EXPORT_MMAP)
            target_link_options(${name} PRIVATE -Xlinker -Map=scene_test.map)
        endif()
    endif ()
    target_link_libraries(${name}
        PUBLIC
        ${GAME_LIBS}
    )
endmacro()

add_target_exe(main)
add_target_exe(level_test)
add_target_exe(scene_test)

if (NOT EMSCRIPTEN)
    if (BUILD_EXTRAS AND NOT RUN_PROFILER)
        add_target_exe(entManager_test)
        add_target_exe(water_test)
        add_target_exe(level_load_test)
        add_target_exe(menu_test)
        add_target_exe(assets_test)
        add_target_exe(particle_test)
        add_target_exe(scene_man_test)
        add_target_exe(level_select_test)
    endif()
    if (BUILD_TESTING)
         find_package(cmocka 1.1.0 REQUIRED)
         add_subdirectory(tests)
    endif()
endif()
