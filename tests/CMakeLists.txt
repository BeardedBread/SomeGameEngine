add_executable(AABBTest test_AABB.c)
target_compile_features(AABBTest PRIVATE c_std_99)
target_link_libraries(AABBTest PRIVATE
    cmocka
    lib_scenes
)

enable_testing()
add_test(NAME AABBTest COMMAND AABBTest)
