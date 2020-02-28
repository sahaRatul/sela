cmake_minimum_required(VERSION 3.0.0)
project(sela VERSION 0.1.0)

# Use C++11
set(CMAKE_CXX_STANDARD 11)
# Require (at least) it
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Don't use e.g. GNU extension (like -std=gnu++11) for portability
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable CTest
include(CTest)
enable_testing()
add_test(NAME SelaTests COMMAND selatests)

# CPack config
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# Make sela executable -----------------
set(SOURCES 
    src/main.cpp
    src/lpc/linear_predictor.cpp
    src/rice/rice_encoder.cpp
    src/rice/rice_decoder.cpp
)
add_executable(sela ${SOURCES})

# Enable Warnings for MSVC/GCC/Clang on Debug Mode
if(CMAKE_BUILD_TYPE MATCHES "Debug")
message(CMAKE_BUILD_TYPE)
    if(MSVC)
        target_compile_options(sela PUBLIC /W4 /WX)
    else()
        target_compile_options(sela PUBLIC -Wall -Wextra -pedantic -Werror)
    endif()
endif()
# --------------------------------------------

#---------------------------------------------
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
option(CODE_COVERAGE "Enable coverage reporting" OFF)
if(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Add required flags (GCC & LLVM/Clang)
    target_compile_options(selatests PUBLIC
        -O0        # no optimization
        --coverage # sets all required flags
    )
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
        target_link_options(selatests PUBLIC Catch --coverage)
    else()
        target_link_libraries(selatests PUBLIC Catch --coverage)
    endif()
endif()
#---------------------------------------------------------------