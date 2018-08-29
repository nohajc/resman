cmake_minimum_required(VERSION 3.0)
project(bin2obj)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})

set(SOURCE_FILES
	main.cpp
	objcompiler.cpp objcompiler.h
	libpacker.cpp libpacker.h)

add_executable(bin2obj ${SOURCE_FILES})

llvm_map_components_to_libnames(LLVM_LIBS core support codegen analysis asmprinter
	x86asmparser x86asmprinter x86codegen x86desc x86info x86utils vectorize)

target_link_libraries(bin2obj ${LLVM_LIBS})
