cmake_minimum_required(VERSION 3.0.0)
project(sela VERSION 0.1.0)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
# Require (at least) it
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Don't use e.g. GNU extension (like -std=gnu++11) for portability
set(CMAKE_CXX_EXTENSIONS OFF)

# Add pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


# Enable CTest
include(CTest)
enable_testing()
add_test(NAME tests COMMAND selatests)

# CPack config
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# Make sela executable -----------------
set(SELA_INCLUDE_DIR src/include)
set(SOURCES 
    src/main.cpp
    src/lpc/linear_predictor.cpp
    src/lpc/residue_generator.cpp
    src/lpc/sample_generator.cpp
    src/rice/rice_encoder.cpp
    src/rice/rice_decoder.cpp
    src/frame/frame_encoder.cpp
    src/frame/frame_decoder.cpp
    src/file/sela_file.cpp
    src/file/wav_file.cpp
    src/sela/encoder.cpp
    src/sela/decoder.cpp
)
add_executable(sela ${SOURCES})
target_include_directories(sela INTERFACE ${SELA_INCLUDE_DIR})
target_link_libraries(sela Threads::Threads)



# Enable Warnings for MSVC/GCC/Clang on Debug Mode
if(CMAKE_BUILD_TYPE MATCHES "Debug")
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
    src/frame/frame_decoder.cpp
    src/frame/frame_encoder.cpp
    test/frametests.cpp
    src/lpc/linear_predictor.cpp
    src/lpc/residue_generator.cpp
    src/lpc/sample_generator.cpp
    test/lpctests.cpp
    src/rice/rice_encoder.cpp
    src/rice/rice_decoder.cpp
    test/ricetests.cpp
    test/main.cpp
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
        target_link_options(selatests PUBLIC --coverage)
    else()
        target_link_libraries(selatests PUBLIC --coverage)
    endif()
endif()
#---------------------------------------------------------------