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

The build auto-copies ONNX Runtime's DLLs. However `onnxruntime_providers_cuda.dll` additionally
links against CUDA 12 / cuDNN 9 runtime libraries that live in your CUDA Toolkit install, **not**
in `build/Release`:

| Library | Belongs to |
|---|---|
| `cudart64_12.dll` | CUDA Runtime |
| `cublas64_12.dll`, `cublasLt64_12.dll` | cuBLAS |
| `cufft64_11.dll` | cuFFT (the `_11` suffix is correct for CUDA 12) |
| `cudnn64_9.dll`, `cudnn_*64_9.dll` | cuDNN 9 |

Two ways to get them next to the executable:

**Option A — let CMake do it.** Set `CUDA_TOOLKIT_ROOT` in `CMakeLists.txt` (top of the file) to
your CUDA 12 install directory, then re-run configure. Every build will copy whatever it finds.

**Option B — run the deploy script.**

    scripts\deploy_cuda_dlls.bat

It looks in `bin\`, `bin\x64\` and `lib\x64\` of the CUDA directory, copies all libraries it finds,
and reports anything missing. Pass a different CUDA path as the first argument if needed.

If CUDA cannot be set up right now, set `model.use_cuda: false` in `config/config.yaml` to fall
back to CPU inference — the program starts and runs without any CUDA DLLs.

## Usage
After building, run the executable:

    ./build/Release/tracking_cpp.exe

The program must be started **from the project root directory**, because `config/config.yaml`,
`models/` and `logs/` are resolved as relative paths.

Press `q` or `ESC` to exit the program.

## Configuration

All tunable parameters live in `config/config.yaml`, grouped into six sections:

| Section | Covers |
|---|---|
| `camera` | device id, capture resolution |
| `video` | output path, fps, `fourcc` codec |
| `model` | ONNX model path, confidence/NMS thresholds, input size, ORT thread count, CUDA on/off and device id |
| `tracking` | detection interval, target class id, IoU gating thresholds, max lost frames, Kalman noise |
| `logging` | log file path, minimum level, console mirroring |
| `display` | window title, show window, draw FPS, profiling interval |

Every key is optional. A missing key falls back to its built-in default and prints a warning;
an out-of-range value is clamped back to a legal value with an explanation. On startup the
effective configuration is printed and written to the log, so a mistyped key name is easy to spot.

A different config file can be passed on the command line:

    ./build/Release/tracking_cpp.exe path/to/other_config.yaml

*(Note: Benchmark data is based on local testing with an RTX 3050 Ti Laptop GPU.)*

## Related Repositories
- Python Implementation: [YOLO-CSRT-Hybrid-Tracker-Python](https://github.com/marityli/YOLO-CSRT-Hybrid-Tracker-Python)