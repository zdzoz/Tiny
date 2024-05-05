add_library(ty_lib
    token.cpp token.h
    parser.cpp parser.h
    ssa.cpp ssa.h
)

target_include_directories(ty_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_precompile_headers(ty_lib PUBLIC pch.h)

add_executable(ty main.cpp)
target_link_libraries(ty ty_lib)
