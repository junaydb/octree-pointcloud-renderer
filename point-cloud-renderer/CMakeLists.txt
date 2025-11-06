cmake_minimum_required(VERSION 3.10)

project(PointCloudRenderer VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG=1")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories(
    ./src
    ./lib
    ./lib/glm
    ./lib/glad2/include
)

file(GLOB_RECURSE CPP_SOURCES 
    "src/*.cpp"
    "src/camera/*.cpp"
    "src/mouse/*.cpp"
    "src/shader-compiler/*.cpp"
    "src/timer/*.cpp"
    "src/point-cloud/*.cpp"
    "src/boundingbox/*.cpp"
    "src/point-cloud/builder/*.cpp"
    "src/octree/*.cpp"
    "lib/miniply/*.cpp"
    "lib/glad2/src/*.c"
)

add_executable(${PROJECT_NAME} ${CPP_SOURCES})
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "${CMAKE_SOURCE_DIR}/lib")

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)
