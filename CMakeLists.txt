cmake_minimum_required(VERSION 3.12)
project(sela VERSION 2.0.2)
INCLUDE(CheckIncludeFiles)

# Verbose toggle
set(CMAKE_VERBOSE_MAKEFILE OFF)
# Use C++11
set(CMAKE_CXX_STANDARD 11)
# Require (at least) it
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Don't use e.g. GNU extension (like -std=gnu++11) for portability
set(CMAKE_CXX_EXTENSIONS OFF)
# Add support for threading library
find_package(Threads REQUIRED)
# Check if ao/ao.h is available or not
CHECK_INCLUDE_FILES(ao/ao.h HAVE_AO_H)
if(NOT HAVE_AO_H)
    CHECK_INCLUDE_FILES(malloc.h HAVE_MALLOC_H)
    CHECK_INCLUDE_FILES(sys/audio.h HAVE_SYS_AUDIO_H)
endif()

# Enable CTest
include(CTest)
enable_testing()
add_test(NAME tests COMMAND selatests)

# CPack config
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# Build libao from source if not found in headers
if(NOT HAVE_AO_H)
    set(
        LIBAO_SOURCES
        external/libao/src/ao_aixs.c
        external/libao/src/ao_au.c
        external/libao/src/ao_null.c
        external/libao/src/ao_raw.c
        external/libao/src/ao_wav.c
        external/libao/src/audio_out.c
        external/libao/src/config.c
    )

    if (WIN32)
        list(APPEND LIBAO_SOURCES external/libao/src/ao_wmm.c)
    endif ()

    if(HAVE_ALSA_ASOUNDLIB_H)
        list(APPEND LIBAO_SOURCES external/libao/src/plugins/alsa/ao_alsa.c)
    endif()

    add_library(ao ${LIBAO_SOURCES})
    target_include_directories(ao PUBLIC external/libao/include)
    target_include_directories(ao PRIVATE external/libao/include/ao)
    target_compile_definitions(ao PRIVATE AO_BUILDING_LIBAO)

    if (WIN32)
        target_compile_definitions(ao PRIVATE HAVE_WMM)
        target_link_libraries(ao PRIVATE Winmm.lib ksuser)
    endif ()

    if (HAVE_SYS_AUDIO_H)
        target_compile_definitions(ao PRIVATE HAVE_SYS_AUDIO_H)
    endif ()
endif()
#---------------------------------------------------

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
    src/sela/player.cpp
)

add_executable(sela ${SOURCES})
target_include_directories(sela PUBLIC ${SELA_INCLUDE_DIR})
if(NOT HAVE_AO_H)
    target_include_directories(sela PRIVATE external/libao/include)
endif()
target_link_libraries(sela Threads::Threads ao)

# Enable Warnings for MSVC/GCC/Clang on Debug Mode
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    if(MSVC)
        target_compile_options(sela PUBLIC /W4 /WX)
    else()
        target_compile_options(sela PUBLIC -Wall -Wextra -pedantic -Werror)
    endif()
endif()
# --------------------------------------------


# Prepare "Catch" library for other executables
set(CATCH_INCLUDE_DIR external/catch2/include)
add_library(Catch INTERFACE)
#--------------------------------------------

# Make test executable------------------------
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
target_include_directories(selatests PRIVATE ${CATCH_INCLUDE_DIR})
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