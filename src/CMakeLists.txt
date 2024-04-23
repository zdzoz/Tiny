add_library(tc_lib
    token.cpp token.h
)

target_include_directories(tc_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_precompile_headers(tc_lib PUBLIC pch.h)

add_executable(tc main.cpp)
target_link_libraries(tc tc_lib)
