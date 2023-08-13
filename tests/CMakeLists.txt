add_executable(AABBTest test_AABB.c)
target_compile_features(AABBTest PRIVATE c_std_99)
target_link_libraries(AABBTest PRIVATE
    cmocka
    lib_scenes
)

add_executable(MemPoolTest test_mempool.c)
target_compile_features(MemPoolTest PRIVATE c_std_99)
target_link_libraries(MemPoolTest PRIVATE
    cmocka
    lib_scenes
)

enable_testing()
add_test(NAME AABBTest COMMAND AABBTest)
add_test(NAME MemPoolTest COMMAND MemPoolTest)
