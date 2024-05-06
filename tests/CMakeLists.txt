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
  test_parser_basic.cpp
  test_parser_intermediate.cpp
  test_parser_complex.cpp
)
target_compile_definitions(ty_tests PRIVATE BASIC=1)
target_compile_definitions(ty_tests PRIVATE INTERMEDIATE=0)
target_compile_definitions(ty_tests PRIVATE COMPLEX=0)

target_compile_definitions(ty_tests PRIVATE BASIC_TESTS="${CMAKE_CURRENT_SOURCE_DIR}/basic_tests/")
target_compile_definitions(ty_tests PRIVATE INTERMEDIATE_TESTS="${CMAKE_CURRENT_SOURCE_DIR}/intermediate_tests/")
target_compile_definitions(ty_tests PRIVATE COMPLEX_TESTS="${CMAKE_CURRENT_SOURCE_DIR}/complex_tests/")
target_link_libraries(ty_tests GTest::gtest_main ty_lib)

include(GoogleTest)
gtest_discover_tests(ty_tests)
