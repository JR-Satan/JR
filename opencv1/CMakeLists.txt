cmake_minimum_required(VERSION 3.10)
project(opencv_project)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

set(CMAKE_BUILD_TYPE Debug)

add_executable(opencvtest1 main.cpp)
target_link_libraries(opencvtest1 ${OpenCV_LIBS})