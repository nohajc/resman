cmake_minimum_required(VERSION 3.1)
project(resman_api)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

set(CMAKE_CXX_STANDARD 11)

find_program(RESCOMP rescomp)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test.o
	COMMAND ${RESCOMP} ${CMAKE_CURRENT_SOURCE_DIR}/test.h -march x86-64 -o ${CMAKE_CURRENT_BINARY_DIR}
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test.h)

set(SOURCE_FILES
	main.cpp test.h ${CMAKE_CURRENT_BINARY_DIR}/test.o)

add_executable(resman_api_test ${SOURCE_FILES})
