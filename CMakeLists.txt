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

#add_definitions(-DsCONFIG_SDK_CG)

include(TestBigEndian)
test_big_endian(IS_BE)
if(IS_BE)
    add_definitions(-DsCONFIG_BE)
else(IS_BE)
    add_definitions(-DsCONFIG_LE)
endif(IS_BE)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR}/util) # it has to be in util, but no

add_definitions(-DsCONFIG_RENDER_OGL2)
#add_definitions(-DsSTF_NVIDIA=sSTF_GLSL)

#include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

add_subdirectory(base)
#ExternalProject_Add(hlsl2glsl GIT_REPOSITORY https://github.com/aras-p/hlsl2glslfork INSTALL_COMMAND "")
#add_subdirectory(hlsl2glsl)
#include_directories(${CMAKE_SOURCE_DIR}/hlsl2glsl/include)
add_subdirectory(util)
add_subdirectory(gui)
add_subdirectory(shadercomp)
add_subdirectory(asc)

option(BUILD_EXAMPLES "build examples" OFF)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif(BUILD_EXAMPLES)