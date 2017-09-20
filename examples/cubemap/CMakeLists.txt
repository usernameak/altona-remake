cmake_minimum_required(VERSION 3.5.0)

include_directories(${CMAKE_BINARY_DIR}/util)

add_custom_command(
    OUTPUT shader.cpp shader.hpp
    COMMAND ${CMAKE_BINARY_DIR}/asc/asc ${CMAKE_CURRENT_SOURCE_DIR}/shader.asc -o ${CMAKE_CURRENT_BINARY_DIR}/shader --gen_cg 
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS asc shader.asc
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(altona_cubemap_example main.cpp shader.cpp)
target_link_libraries(altona_cubemap_example altona_base altona_util altona_gui)