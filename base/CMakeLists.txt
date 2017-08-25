cmake_minimum_required(VERSION 3.5.0)

set(PLATDEP_FILES_BASE )

find_package(X11 REQUIRED COMPONENTS X11 Xft Xrender)
include_directories(${X11_INCLUDE_PATH} ${X11_Xrender_INCLUDE_PATH} ${X11_Xft_INCLUDE_PATH})

if(WIN32)
    
elseif(UNIX)
    set(PLATDEP_FILES_BASE system_linux.cpp windows_xlib.cpp)
endif(WIN32)

add_library(altona_base SHARED graphics.cpp math.cpp sound_common.cpp sound_pulseaudio.cpp system.cpp
    input2.cpp types.cpp types2.cpp devices_win.cpp serialize.cpp windows.cpp graphics_ogl2.cpp glprocs.c ${PLATDEP_FILES_BASE})
target_link_libraries(altona_base GL pthread ${X11_LIBRARIES} ${X11_Xrender_LIB} ${X11_Xft_LIB})