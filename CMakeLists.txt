cmake_minimum_required(VERSION 3.0.0)
project(ffmpeg_sample VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(fitting fitting.cpp)

target_include_directories(
    fitting
    PUBLIC
    "/usr/include/opencv4"
)

target_link_directories(
    fitting
    PUBLIC
    "/usr/lib/x86_64-linux-gnu"
)

target_link_libraries(
    fitting
    "opencv_core"
    "opencv_highgui"
    "opencv_imgproc"
    "opencv_ml"
    "opencv_imgcodecs"
    "m"
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
