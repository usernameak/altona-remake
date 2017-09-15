cmake_minimum_required(VERSION 3.5.0)

configure_file(recttoy.wire.txt recttoy.wire.txt COPYONLY)
add_executable(altona_recttoy_example main.cpp)
target_link_libraries(altona_recttoy_example altona_base altona_util altona_gui)