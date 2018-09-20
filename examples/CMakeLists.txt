cmake_minimum_required(VERSION 3.1)
project(resman_api)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

set(CMAKE_CXX_STANDARD 11)

find_program(RESCOMP rescomp PATHS ../build)
message(STATUS "Found resource compiler: ${RESCOMP}")

set(RESDEFS_IN ${CMAKE_CURRENT_SOURCE_DIR}/resdefs.h)
set(RESDEFS_OUT ${CMAKE_CURRENT_BINARY_DIR}/resdefs.o)

add_custom_command(OUTPUT ${RESDEFS_OUT}
	COMMAND ${RESCOMP} ${RESDEFS_IN} -o ${RESDEFS_OUT} --
	DEPENDS ${RESDEFS_IN})

set(SOURCE_FILES
	main.cpp ${RESDEFS_IN} ${RESDEFS_OUT})

add_executable(resman_api_test ${SOURCE_FILES})
