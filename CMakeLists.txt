cmake_minimum_required(VERSION 3.0.0)
project(sela VERSION 0.1.0)

include(CTest)
enable_testing()

include_directories(include)

add_executable(
    sela
    src/main.cpp
    src/lpc/linear_predictor.cpp
    src/rice/rice_encoder.cpp
    src/rice/rice_decoder.cpp
)

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
