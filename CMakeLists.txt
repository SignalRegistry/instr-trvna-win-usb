cmake_minimum_required(VERSION 3.0.0)
project(main VERSION 0.1.0)

# Switch to C++ compiler to use #import directive
SET_SOURCE_FILES_PROPERTIES( main.c PROPERTIES LANGUAGE CXX )
# set (CMAKE_CXX_STANDARD 11)

include_directories( include )
file(GLOB SOURCES "src/*.cpp" "src/*.c")

add_executable(instr-client-trvna-win main.c ${SOURCES})
# target_link_libraries(instr-client-trvna-win ${PROJECT_LINK_LIBS})