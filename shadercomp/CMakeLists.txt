cmake_minimum_required(VERSION 3.5.0)

add_library(altona_shadercomp SHARED asc_doc.cpp asc_parser.cpp asc_output.cpp
    shaderdis.cpp compile_cg.cpp)
target_link_libraries(altona_shadercomp altona_base hlsl2glsl Cg)