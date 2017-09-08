cmake_minimum_required(VERSION 3.5.0)

add_executable(asc main.cpp parse.cpp output.cpp new.cpp ${CMAKE_SOURCE_DIR}/util/scanner.cpp)
target_link_libraries(asc altona_base altona_shadercomp)
SET_TARGET_PROPERTIES(asc PROPERTIES COMPILE_FLAGS -DsCONFIG_OPTION_SHELL=1)