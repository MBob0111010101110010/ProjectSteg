cmake_minimum_required(VERSION "3.21.1")

project("ProjectSteg")

find_package( OpenCV REQUIRED )

add_executable(ProjectSteg ProjectSteg.cpp)

target_link_libraries( ProjectSteg ${OpenCV_LIBS} )

find_package (OpenCV 4.0.0 REQUIRED)
include_directories ("/usr/include/opencv2/")