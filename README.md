# YOLO + Multi-Object Tracking System (C++)

A real-time multi-object tracking system implemented in C++, combining YOLO11n object detection with multi-target tracking for robust and efficient target following.

## Features
- **YOLO11n ONNX Inference**: High-performance object detection using ONNX Runtime with CUDA acceleration.
- **Letterbox Preprocessing**: Maintains aspect ratio during scaling to preserve detection accuracy.
- **Multi-Object Tracking (MOT)**: Tracks multiple targets simultaneously using IoU matching and independent trackers (KCF/CSRT).
- **Multi-Threaded Pipeline**: Separates camera reading and inference into different threads for maximum FPS (upcoming).
- **Performance Profiling**: Built-in timers for preprocessing, inference, and post-processing.

## Prerequisites
- **OS**: Windows 10/11
- **Compiler**: Visual Studio 2022 (MSVC)
- **Build System**: CMake (>= 3.15)
- **Dependencies**:
  - OpenCV (>= 4.x, tested with 5.0.0)
  - ONNX Runtime (CUDA 12 version, tested with 1.20.1)
  - CUDA Toolkit (>= 12.x) + cuDNN (>= 9.x)
  - yaml-cpp (via vcpkg)

## Model Preparation
Due to GitHub file size limits, large model files are not included in this repository.
**Please download or export the YOLO11n ONNX model and place it in the `models/` directory.**
Supported model formats: `.pt`, `.onnx`, `.engine` (these are ignored by `.gitignore`).

## Build Instructions

### 1. Configure the project

    cmake -S . -B build

### 2. Build the project

    cmake --build build --config Release

### 3. Prepare runtime DLLs
Copy necessary CUDA DLLs (e.g., `cudnn64_9.dll`, `cublasLt64_12.dll`) into `build/Release/` if they are not automatically found.

## Usage
After building, run the executable:

    ./build/Release/tracking_cpp.exe

Press `q` or `ESC` to exit the program.

*(Note: Benchmark data is based on local testing with an RTX 3050 Ti Laptop GPU.)*

## Related Repositories
- Python Implementation: [YOLO-CSRT-Hybrid-Tracker-Python](https://github.com/marityli/YOLO-CSRT-Hybrid-Tracker-Python)