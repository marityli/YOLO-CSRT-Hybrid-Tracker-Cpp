# YOLO + CSRT Hybrid Object Tracking System (C++)

A real-time object tracking system implemented in C++, combining YOLO object detection with CSRT tracking for robust and efficient target following.

## Prerequisites
- CMake (>= 3.10)
- C++ Compiler (MSVC, GCC, or Clang)
- vcpkg (for dependency management)

## Dependencies
Dependencies are managed by vcpkg. Check `vcpkg.json` for details.

## Build Instructions

**1. Configure the project**

    cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=[path_to_vcpkg]/scripts/buildsystems/vcpkg.cmake

**2. Build the project**

    cmake --build build --config Release

## Usage
After building, run the executable:

    ./build/Release/ObjectTracking.exe

## Related Repositories
- Python Implementation: [YOLO-CSRT-Hybrid-Tracker-Python](https://github.com/marityli/YOLO-CSRT-Hybrid-Tracker-Python)