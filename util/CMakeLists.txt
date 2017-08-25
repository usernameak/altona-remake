cmake_minimum_required(VERSION 3.5.0)

add_library(altona_util SHARED camera.cpp effect.cpp image.cpp musicplayer.cpp
    painter.cpp scanner.cpp scanconfig.cpp shaders.cpp animation.cpp
    movieplayer.cpp perfmon.cpp taskscheduler.cpp ipp.cpp rasterizer.cpp stb_image.c
    )
target_link_libraries(altona_util altona_base)