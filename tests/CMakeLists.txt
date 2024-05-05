include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest
  GIT_TAG        f8d7d77c06936315286eb55f8de22cd23c188571 # v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(ty_tests
  test_tokenizer.cpp
)

target_compile_definitions(ty_tests PRIVATE TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src/")
target_link_libraries(ty_tests GTest::gtest_main ty_lib)

include(GoogleTest)
gtest_discover_tests(ty_tests)
