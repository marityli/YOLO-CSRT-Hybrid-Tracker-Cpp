cmake_minimum_required(VERSION 3.15)

project(OpenCVTracking)

if (MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OpenCV_DIR "D:/OpenCV/opencv/build_contrib/install")
set(ONNXRUNTIME_ROOT "D:/ONNX/onnxruntime-win-x64-gpu-1.20.1")

# ============================================================================
#  CUDA / cuDNN 运行库来源
#
#  onnxruntime_providers_cuda.dll 依赖 CUDA 12 + cuDNN 9 的运行库，这些库不在
#  build\Release 里。本机没有单独装 CUDA 12 Toolkit，但 conda 环境的 PyTorch
#  自带了整套 CUDA 12 + cuDNN 9 运行库（这正是之前 GPU 推理能跑起来的原因），
#  所以直接从 torch\lib 拷贝。
#
#  换机器 / 换 conda 环境时改这一行即可。若该 env 的 torch 是 CPU-only 版本，
#  目录里就没有 CUDA 库，下面的拷贝会自动跳过并打印提示，不会中断构建。
# ============================================================================
set(TORCH_LIB_DIR "D:/Anaconda_envs/envs/opencv_tracking/Lib/site-packages/torch/lib")

# OpenCV 编译时启用了 AVIF 支持，opencv_imgcodecs500.dll 会额外加载 avif.dll。
# 该库不在 OpenCV 的 bin 目录里，本机可从 Anaconda 的 Library\bin 取到。
set(ANACONDA_ROOT "D:/Anaconda")


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
    "include/Logger.h" "src/Logger.cpp"
    "include/Config.h"  "src/Config.cpp"
    "include/Tracker.h" "src/Tracker.cpp"
)

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
    # ---------------- ONNX Runtime DLLs ----------------
    file(GLOB ONNXRUNTIME_DLLS "${ONNXRUNTIME_ROOT}/lib/*.dll")
    add_custom_command(TARGET tracking_cpp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${ONNXRUNTIME_DLLS}
        $<TARGET_FILE_DIR:tracking_cpp>
        COMMENT "Copying ONNX Runtime DLLs..."
    )

    # ---------------- CUDA / cuDNN runtime DLLs ----------------
    #
    # 必须凑齐的整套（ORT 的 provider 按名字直接加载，缺一个就加载失败）：
    #
    #   cudart64_12 / cublas64_12 / cublasLt64_12 / cufft64_11 / cudnn64_9
    #       —— provider 直接引用的 5 个；
    #          cufft 的 "_11" 后缀是正常的，它属于 CUDA 12 系列
    #
    #   cudnn_ops64_9 / cudnn_cnn64_9 / cudnn_adv64_9 / cudnn_graph64_9
    #   cudnn_engines_precompiled64_9 / cudnn_engines_runtime_compiled64_9
    #   cudnn_heuristic64_9
    #       —— cudnn64_9.dll 内部要加载的子库，不拷会在首次推理时报错
    #
    #   zlibwapi
    #       —— cuDNN / cuBLAS 的压缩依赖
    #
    # 其余是「建议补上」的库（nvrtc / nvJitLink / curand / cusolver / cusparse /
    # libiomp5md），用于 cuBLAS 内部 JIT 和 cuDNN 部分高级算法。
    #
    # 每个库单独判断存在性：缺哪个只打印哪个，不影响其它库的拷贝。
    # ============================================================
    set(CUDA_DLL_NAMES
        cudart64_12.dll
        cublas64_12.dll
        cublasLt64_12.dll
        cufft64_11.dll
        cudnn64_9.dll
        cudnn_ops64_9.dll
        cudnn_cnn64_9.dll
        cudnn_adv64_9.dll
        cudnn_graph64_9.dll
        cudnn_engines_precompiled64_9.dll
        cudnn_engines_runtime_compiled64_9.dll
        cudnn_heuristic64_9.dll
        zlibwapi.dll
        nvrtc64_120_0.dll
        nvJitLink_120_0.dll
        curand64_10.dll
        cusolver64_11.dll
        cusolverMg64_11.dll
        cusparse64_12.dll
        libiomp5md.dll
    )

    set(CUDA_DLLS "")
    set(CUDA_DLLS_MISSING "")
    foreach(_dll IN LISTS CUDA_DLL_NAMES)
        if (EXISTS "${TORCH_LIB_DIR}/${_dll}")
            list(APPEND CUDA_DLLS "${TORCH_LIB_DIR}/${_dll}")
        else()
            list(APPEND CUDA_DLLS_MISSING "${_dll}")
        endif()
    endforeach()

    if (CUDA_DLLS)
        list(LENGTH CUDA_DLLS _n)
        add_custom_command(TARGET tracking_cpp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CUDA_DLLS}
            $<TARGET_FILE_DIR:tracking_cpp>
            COMMENT "Copying ${_n} CUDA/cuDNN runtime DLLs..."
        )
    endif()

    if (CUDA_DLLS_MISSING)
        list(LENGTH CUDA_DLLS_MISSING _nm)
        message(STATUS "CUDA DLL(s) not found in '${TORCH_LIB_DIR}': ${CUDA_DLLS_MISSING} (${_nm} of them)")
        message(STATUS "If GPU inference fails to start: fix TORCH_LIB_DIR, or set model.use_cuda=false in config/config.yaml to run on CPU.")
    endif()

    # ---------------- OpenCV 的 AVIF 解码后端 ----------------
    #
    # 本机的 OpenCV 5.0.0 编译时启用了 AVIF 支持，opencv_imgcodecs500.dll
    # 因此会在加载时去要 avif.dll。这个库既不在 OpenCV 的 bin 目录里，
    # 也不在系统目录里，缺了会导致进程启动直接失败（0xC0000135 找不到 DLL），
    # 而报错窗口往往一闪而过、看不到缺的是哪个 —— 很难排查。
    #
    # avif.dll 自己还依赖 dav1d.dll，两个必须成对拷贝。
    # 实测可用来源：Anaconda 的 Library\bin
    # 换机器 / 换环境后如果启动直接闪退，优先检查这里。
    set(AVIF_DLL_NAMES avif.dll dav1d.dll)

    set(AVIF_DLLS "")
    set(AVIF_DLLS_MISSING "")
    foreach(_name IN LISTS AVIF_DLL_NAMES)
        if (EXISTS "${ANACONDA_ROOT}/Library/bin/${_name}")
            list(APPEND AVIF_DLLS "${ANACONDA_ROOT}/Library/bin/${_name}")
        else()
            list(APPEND AVIF_DLLS_MISSING "${_name}")
        endif()
    endforeach()

    if (AVIF_DLLS)
        add_custom_command(TARGET tracking_cpp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${AVIF_DLLS}
            $<TARGET_FILE_DIR:tracking_cpp>
            COMMENT "Copying avif/dav1d (OpenCV imgcodecs dependency)..."
        )
    endif()

    if (AVIF_DLLS_MISSING)
        message(STATUS
            "Not found in '${ANACONDA_ROOT}/Library/bin': ${AVIF_DLLS_MISSING}. "
            "OpenCV imgcodecs needs them; without them the exe exits with 0xC0000135. "
            "Set ANACONDA_ROOT in CMakeLists.txt, or add their directory to PATH.")
    endif()
endif()
