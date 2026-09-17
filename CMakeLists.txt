cmake_minimum_required(VERSION 3.15)

project(OpenCVTracking)

if (MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OpenCV_DIR "D:/OpenCV/opencv/build_contrib/install")
set(ONNXRUNTIME_ROOT "D:/ONNX/onnxruntime-win-x64-gpu-1.20.1")


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
    src/YOLODetector.cpp
    src/MultiTracker.cpp
 "include/Logger.h" "src/Logger.cpp" "include/Config.h" "src/Config.cpp" "include/Tracker.h" "src/Tracker.cpp")

target_link_libraries(tracking_cpp
    PRIVATE
    ${OpenCV_LIBS}
    yaml-cpp::yaml-cpp
    "${ONNXRUNTIME_ROOT}/lib/onnxruntime.lib"
)

target_include_directories(tracking_cpp
    PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
    "${ONNXRUNTIME_ROOT}/include"
)

if (WIN32)
    file(GLOB ONNXRUNTIME_DLLS "${ONNXRUNTIME_ROOT}/lib/*.dll")
    add_custom_command(TARGET tracking_cpp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${ONNXRUNTIME_DLLS}
        $<TARGET_FILE_DIR:tracking_cpp>
        COMMENT "Copying ONNX Runtime DLLs..."
    )
endif()