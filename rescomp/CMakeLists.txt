cmake_minimum_required(VERSION 3.0)
project(rescomp)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

find_package(Clang REQUIRED)
include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${CLANG_DEFINITIONS})

set(COMMON "../common")

set(SOURCE_FILES
	main.cpp
	${COMMON}/objcompiler.cpp ${COMMON}/objcompiler.h
	${COMMON}/libpacker.cpp ${COMMON}/libpacker.h
	${COMMON}/fileio.cpp ${COMMON}/fileio.h)

add_executable(rescomp ${SOURCE_FILES})

#llvm_map_components_to_libnames(LLVM_LIBS core support codegen analysis asmprinter
#	x86asmparser x86asmprinter x86codegen x86desc x86info x86utils vectorize)

set(CLANGTOOL_LIBS
   clangFrontend
   clangAST
   clangTooling)

#target_link_libraries(rescomp ${LLVM_LIBS} ${CLANGTOOL_LIBS})
target_link_libraries(rescomp LLVM ${CLANGTOOL_LIBS})