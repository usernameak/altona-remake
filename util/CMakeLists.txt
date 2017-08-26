cmake_minimum_required(VERSION 3.5.0)

STRING (REGEX MATCH "Linux" PROJECT_OS_LINUX ${CMAKE_SYSTEM_NAME})

if(WIN32)
    add_custom_command(
        OUTPUT shaders.cpp shaders.hpp
        COMMAND ${CMAKE_SOURCE_DIR}/tools/asc.exe ${CMAKE_CURRENT_SOURCE_DIR}/shaders.asc
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
elseif(PROJECT_OS_LINUX)
    add_custom_command(
        OUTPUT shaders.cpp shaders.hpp
        COMMAND ${CMAKE_SOURCE_DIR}/tools/asc ${CMAKE_CURRENT_SOURCE_DIR}/shaders.asc -o ${CMAKE_CURRENT_BINARY_DIR}/shaders
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
endif(WIN32)

add_library(altona_util SHARED shaders.hpp camera.cpp effect.cpp image.cpp musicplayer.cpp
    painter.cpp scanner.cpp scanconfig.cpp shaders.cpp animation.cpp
    movieplayer.cpp perfmon.cpp taskscheduler.cpp ipp.cpp rasterizer.cpp stb_image.cpp
    
    )
target_link_libraries(altona_util altona_base)