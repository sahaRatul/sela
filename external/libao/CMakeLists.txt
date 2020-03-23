cmake_minimum_required(VERSION 3.12)
project(libao)
INCLUDE(CheckIncludeFiles)

CHECK_INCLUDE_FILES(malloc.h HAVE_MALLOC_H)
CHECK_INCLUDE_FILES(sys/audio.h HAVE_SYS_AUDIO_H)

set(
        LIBAO_SOURCES
        src/ao_aixs.c
        src/ao_au.c
        src/ao_null.c
        src/ao_raw.c
        src/ao_wav.c
        src/audio_out.c
        src/config.c
)

if (WIN32)
    list(APPEND LIBAO_SOURCES src/ao_wmm.c)
endif ()

add_library(libao ${LIBAO_SOURCES})
target_include_directories(libao PUBLIC include)
target_include_directories(libao PRIVATE include/ao)
target_compile_definitions(libao PRIVATE AO_BUILDING_LIBAO)

if (WIN32)
    target_compile_definitions(libao PRIVATE HAVE_WMM)
    target_link_libraries(libao PRIVATE Winmm.lib ksuser)
endif ()

if (HAVE_SYS_AUDIO_H)
    target_compile_definitions(libao PRIVATE HAVE_SYS_AUDIO_H)
endif ()

add_subdirectory(example)
