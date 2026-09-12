cmake_minimum_required(VERSION 3.15)

project(OpenCVTracking)

if (MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OpenCV_DIR "D:/OpenCV/opencv/build_contrib/install")


find_package(OpenCV REQUIRED)
find_package(yaml-cpp REQUIRED)

add_executable(tracking_cpp
    src/main.cpp
    src/Camera.cpp
    src/FPS.cpp
    src/VideoRecorder.cpp
    src/Logger.cpp
    src/Config.cpp
    src/Tracker.cpp
 "include/Logger.h" "src/Logger.cpp" "include/Config.h" "src/Config.cpp" "include/Tracker.h" "src/Tracker.cpp")

target_link_libraries(tracking_cpp
    PRIVATE
    ${OpenCV_LIBS}
    yaml-cpp::yaml-cpp
)

target_include_directories(tracking_cpp
    PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
)