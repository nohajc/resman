cmake_minimum_required(VERSION 3.0)
project(resman_api)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

set(SOURCE_FILES
	main.cpp test.h)

add_executable(resman_api_test ${SOURCE_FILES})
