project(gameoflife)
cmake_minimum_required(VERSION 3.2)

if (NOT DEFINED FF_ROOT)
	if (NOT EQUAL $ENV{FF_ROOT} "")
		MESSAGE(WARNING "FF_ROOT variable not set")
		EXIT()
	endif (NOT EQUAL $ENV{FF_ROOT} "")
	set(FF_ROOT "$ENV{FF_ROOT}")
endif(NOT DEFINED FF_ROOT)


include_directories( ${FF_ROOT}/ff)
include_directories( ${CMAKE_SOURCE_DIR}/include)

find_package( OpenCV )

if (OpenCV_FOUND)
	include_directories(${OpenCV_INCLUDE_DIRS})
	option( OPENCV_SCREEN "enable opencv" ON)
else(OpenCV_FOUND)
	option( OPENCV_SCREEN "disable opencv" OFF)
endif(OpenCV_FOUND)

configure_file(
		gol_config.hpp.in
		gol_config.hpp)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -g -I${FF_ROOT} -pthread -fopenmp -msse2 -ftree-vectorizer-verbose=5")
set(SOURCE_FILES)
add_executable(gol src/main_fastflow.cpp ${SOURCE_FILES} src/opencv_visualizer.hpp src/empty_visualizer.hpp src/terminal_visualizer.hpp src/barrier.hpp src/gol_farm.hpp)
target_link_libraries( gol ${OpenCV_LIBS} )