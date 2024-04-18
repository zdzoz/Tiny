add_executable(tc
    main.cpp
    token.cpp token.h
)

target_precompile_headers(tc PRIVATE pch.h)
