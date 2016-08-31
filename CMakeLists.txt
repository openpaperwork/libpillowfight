cmake_minimum_required(VERSION 3.0)


add_definitions(-DNO_PYTHON)
include_directories(include)

add_library(libpillowfight SHARED
	src/pypillowfight/_ace.c
	src/pypillowfight/_blackfilter.c
	src/pypillowfight/_blurfilter.c
	src/pypillowfight/_border.c
	src/pypillowfight/_canny.c
	src/pypillowfight/_grayfilter.c
	src/pypillowfight/_masks.c
	src/pypillowfight/_noisefilter.c
	src/pypillowfight/_sobel.c
	src/pypillowfight/util.c
)
