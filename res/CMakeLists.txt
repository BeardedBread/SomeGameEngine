add_executable(rres_packer
    rres_packer.c
)
set_target_properties(rres_packer
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
)

target_link_libraries(rres_packer
    PRIVATE
    lib_engine
)

