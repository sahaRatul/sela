cmake_minimum_required(VERSION 3.0.0)
project(sela VERSION 0.1.0)

# Enable CTest
include(CTest)
enable_testing()
add_test(NAME SelaTests COMMAND selatests)

# CPack config
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# Make sela executable
set(SOURCES 
    src/main.cpp
    src/lpc/linear_predictor.cpp
    src/rice/rice_encoder.cpp
    src/rice/rice_decoder.cpp
)
add_executable(sela ${SOURCES})

# Prepare "Catch" library for other executables
set(CATCH_INCLUDE_DIR test/include)
add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${CATCH_INCLUDE_DIR})

# Make test executable
set(TEST_SOURCES 
    src/rice/rice_encoder.cpp
    src/rice/rice_decoder.cpp
    test/ricetests.cpp
)
add_executable(selatests ${TEST_SOURCES})
target_link_libraries(selatests Catch)

# Enable Warnings for MSVC/GCC/Clang
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()
