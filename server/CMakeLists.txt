cmake_minimum_required (VERSION 3.1)
project (tag_server)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../shared/cmake")

find_package(ENet REQUIRED)

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.c")
#source groups split files into the correct hierarchy in VS
source_group(src
    REGULAR_EXPRESSION ${CMAKE_SOURCE_DIR}/src/*)

add_executable(tag_server ${SOURCE_FILES})

target_include_directories(tag_server
    PRIVATE ${CMAKE_SOURCE_DIR}/src
    PRIVATE ${CMAKE_SOURCE_DIR}/include
    PRIVATE ${CMAKE_SOURCE_DIR}/../shared/include
    PRIVATE ${ENet_INCLUDE_DIRS})

if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    #g++ options
    target_compile_options(tag_server
        PRIVATE -Wall
        PRIVATE -Wextra
        PRIVATE -pedantic)
endif()

target_link_libraries(tag_server
	${ENet_LIBRARIES})
