cmake_minimum_required(VERSION 3.5.0)

if(WIN32)
    if(MSVC) 
        add_definitions(-DsCONFIG_COMPILER_MSC)
    endif(MSVC)
    add_definitions(-DsCONFIG_SYSTEM_WINDOWS)
elseif(UNIX)
    add_definitions(-DsCONFIG_SYSTEM_LINUX)
    include_directories(/usr/include/freetype2)
endif(WIN32)

include(TestBigEndian)
test_big_endian(IS_BE)
if(IS_BE)
    add_definitions(-DsCONFIG_BE)
else(IS_BE)
    add_definitions(-DsCONFIG_LE)
endif(IS_BE)

include_directories(${CMAKE_SOURCE_DIR})

add_definitions(-DsCONFIG_RENDER_OGL2)
add_definitions(-DsSTF_NVIDIA=sSTF_GLSL)

add_subdirectory(base)
add_subdirectory(util)
add_subdirectory(gui)
add_subdirectory(window_example)