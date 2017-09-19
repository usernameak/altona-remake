cmake_minimum_required(VERSION 3.5.0)

include_directories(${CMAKE_BINARY_DIR}/util)

add_executable(altona_cube_example main.cpp)
target_link_libraries(altona_cube_example altona_base altona_util altona_gui)