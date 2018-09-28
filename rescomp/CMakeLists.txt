cmake_minimum_required(VERSION 3.0)
project(rescomp)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

find_package(LLVM 6 REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

IF(DEFINED LINKING AND LINKING STREQUAL "static")
	set(LINKER_EXTRA_FLAG "-static")
ELSE()
   set(LINKER_EXTRA_FLAG "")
ENDIF()

find_package(Clang 6 QUIET)

IF(APPLE OR Clang_FOUND)
	link_directories("${CMAKE_PREFIX_PATH}/lib")
ELSE()
	link_directories("/usr/lib/llvm-6.0/lib")
ENDIF()

set(COMMON "../common")

set(SOURCE_FILES
	main.cpp
	${COMMON}/objcompiler.cpp ${COMMON}/objcompiler.h
	${COMMON}/libpacker.cpp ${COMMON}/libpacker.h
	${COMMON}/fileio.cpp ${COMMON}/fileio.h)

add_executable(rescomp ${SOURCE_FILES})

llvm_map_components_to_libnames(LLVM_LIBS core support codegen analysis asmprinter
	x86asmparser x86asmprinter x86codegen x86desc x86info x86utils vectorize option)

set(CLANGTOOL_LIBS
   clangFrontend
   clangSerialization
   clangDriver
   clangParse
   clangSema
   clangAnalysis
   clangAST
   clangBasic
   clangEdit
   clangLex
   clangTooling)

IF(Clang_FOUND)
   include_directories(${CLANG_INCLUDE_DIRS})
   add_definitions(${CLANG_DEFINITIONS})

   target_link_libraries(rescomp LLVM ${CLANGTOOL_LIBS})
ELSE()
   target_link_libraries(rescomp ${CLANGTOOL_LIBS} ${LLVM_LIBS} ${LINKER_EXTRA_FLAG})
ENDIF()
