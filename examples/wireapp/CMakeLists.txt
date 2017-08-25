cmake_minimum_required(VERSION 3.5.0)

configure_file(wireapp.wire.txt wireapp.wire.txt COPYONLY)
add_executable(altona_wireapp_example main.cpp gui.cpp doc.cpp)
target_link_libraries(altona_wireapp_example altona_base altona_util altona_gui)