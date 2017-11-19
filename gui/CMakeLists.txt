cmake_minimum_required(VERSION 3.5.0)

set (CMAKE_EXTRA_INCLUDE_FILES ${CMAKE_BINARY_DIR}/util/shaders.hpp)
message(${CMAKE_BINARY_DIR})

add_library(altona_gui SHARED gui.cpp manager.cpp window.cpp wire.cpp
    borders.cpp controls.cpp dialog.cpp frames.cpp listwindow.cpp
    overlapped.cpp textwindow.cpp timeline.cpp color.cpp tabs.cpp
    )
target_link_libraries(altona_gui altona_base altona_util)