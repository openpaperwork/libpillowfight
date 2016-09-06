cmake_minimum_required(VERSION 3.0)

project(libpillowfight)
set(CMAKE_BUILD_TYPE Release)

add_definitions(-DNO_PYTHON)
include_directories(include)

add_library(libpillowfight SHARED
	src/pillowfight/_ace.c
	src/pillowfight/_blackfilter.c
	src/pillowfight/_blurfilter.c
	src/pillowfight/_border.c
	src/pillowfight/_canny.c
	src/pillowfight/_gaussian.c
	src/pillowfight/_grayfilter.c
	src/pillowfight/_masks.c
	src/pillowfight/_noisefilter.c
	src/pillowfight/_sobel.c
	src/pillowfight/util.c
)
