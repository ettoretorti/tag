cmake_minimum_required (VERSION 3.1)
project (tag_client)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../shared/cmake")

find_package(ENet REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.c")
#source groups split files into the correct hierarchy in VS
source_group(src
    REGULAR_EXPRESSION ${CMAKE_SOURCE_DIR}/src/*)
source_group(src\\geom
    REGULAR_EXPRESSION ${CMAKE_SOURCE_DIR}/src/geom/*)
source_group(src\\glh
    REGULAR_EXPRESSION ${CMAKE_SOURCE_DIR}/src/glh/*)

add_executable(tag_client ${SOURCE_FILES})

target_include_directories(tag_client
    PRIVATE ${CMAKE_SOURCE_DIR}/src
    PRIVATE ${CMAKE_SOURCE_DIR}/include
    PRIVATE ${CMAKE_SOURCE_DIR}/../shared/include
    PRIVATE ${ENet_INCLUDE_DIRS}
    PRIVATE ${SDL2_INCLUDE_DIR}
    PRIVATE ${GLEW_INCLUDE_DIRS})

target_compile_definitions(tag_client
    PUBLIC NOMINMAX)

if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    #g++ options
    target_compile_options(tag_client
        PRIVATE -Wall
        PRIVATE -Wextra
        PRIVATE -pedantic)
endif()

target_link_libraries(tag_client
    ${ENet_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${SDL2_LIBRARY})
