cmake_minimum_required(VERSION 2.8)
project(pngquant C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -O3 -lm")
add_definitions(-DNDEBUG)

if (APPLE AND CMAKE_OSX_ARCHITECTURES)
    set(TARGET_ARCH ${CMAKE_OSX_ARCHITECTURES})
else()
    set(TARGET_ARCH ${CMAKE_SYSTEM_PROCESSOR})
endif()

if(TARGET_ARCH MATCHES "^arm64" OR TARGET_ARCH MATCHES "^aarch64")
    set(IS_ARM "ON")
endif()

if (IS_ARM)
    add_definitions(-DUSE_SSE=0)
    # arm png
    add_definitions(-DPNG_ARM_NEON_OPT=2)
    set(libpng_arm_sources
            libpng/arm/arm_init.c
            libpng/arm/filter_neon.S
            libpng/arm/filter_neon_intrinsics.c)

    message("====== PNG_ARM_NEON ======")
else()
    add_definitions(-DPNG_ARM_NEON_OPT=0)

    add_definitions(-DUSE_SSE=1)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse3")

    find_package(OpenMP)
    if(OPENMP_FOUND)
        # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    endif()
endif()

include_directories(
        .
        lib
        libpng
        zlib
        libadvpng
)

file(GLOB QUA_SOURCE lib/*.c)
file(GLOB PNG_SOURCE libpng/*.c)
file(GLOB ZLIB_SOURCE zlib/*.c)
file(GLOB_RECURSE ADVPNG_SOURCE libadvpng/*.c)

set(PNG_SOURCE ${PNG_SOURCE} ${libpng_arm_sources})

add_library(imagequant ${QUA_SOURCE})
add_library(rwpng rwpng.c ${PNG_SOURCE} ${ZLIB_SOURCE})
add_library(${PROJECT_NAME} STATIC quant.c rwpng.c ${PNG_SOURCE} ${ZLIB_SOURCE} ${QUA_SOURCE} ${ADVPNG_SOURCE})
