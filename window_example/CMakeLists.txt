cmake_minimum_required(VERSION 3.5.0)


add_executable(altona_window_example main.cpp)
target_link_libraries(altona_window_example altona_base altona_util altona_gui)