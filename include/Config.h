#pragma once

#include <string>

// ============================================================
//  配置分组结构体
//  每个字段都带默认值：config.yaml 里缺哪个键，就用这里的默认值，
//  并在控制台打印一条告警，而不是直接抛异常。
// ============================================================

// ---------- 相机采集 ----------
struct CameraConfig
{
	int id = 0;
	int width = 640;
	int height = 480;
};

// ---------- 视频录制 ----------
struct VideoConfig
{
	std::string path = "results/tracking_result.mp4";
	double fps = 30.0;
	std::string fourcc = "mp4v";
};

// ---------- 模型 / ONNX Runtime ----------
struct ModelConfig
{
	std::string path = "models/yolo11n.onnx";
	float confidence_threshold = 0.5f;
	float nms_threshold = 0.45f;
	int input_width = 640;
	int input_height = 640;
	int intra_op_threads = 0;      // 0 = 由 ONNX Runtime 自动决定
	bool use_cuda = true;
	int cuda_device_id = 0;
	int profile_interval = 30;     // 检测器内部耗时打印间隔（帧）
};

// ---------- 卡尔曼滤波噪声 ----------
struct KalmanConfig
{
	float process_noise = 1e-2f;
	float measurement_noise = 1e-1f;
	float error_cov_post = 1.0f;
};

// ---------- 检测 / 跟踪调度 ----------
struct TrackingConfig
{
	int detection_interval = 15;
	int target_class_id = 0;
	float iou_threshold = 0.3f;
	float reinit_iou_threshold = 0.6f;
	int max_lost_frames = 30;
	KalmanConfig kalman;
};

// ---------- 日志 ----------
struct LoggingConfig
{
	std::string path = "logs/tracking.log";
	std::string level = "INFO";     // DEBUG / INFO / WARNING / ERROR
	bool console = true;
};

// ---------- 显示 / 性能分析 ----------
struct DisplayConfig
{
	std::string window_name = "YOLO + CSRT Hybrid Tracking";
	bool show_window = true;
	bool draw_fps = true;
	int profile_interval = 10;      // 主循环分段耗时打印间隔（帧）
};

// ============================================================
//  Config：读取 config.yaml，返回上面这些分组
// ============================================================
class Config
{
private:
	CameraConfig   camera_config;
	VideoConfig    video_config;
	ModelConfig    model_config;
	TrackingConfig tracking_config;
	LoggingConfig  logging_config;
	DisplayConfig  display_config;

	void validate();

public:
	explicit Config(const std::string& path);

	const CameraConfig&   camera()   const;
	const VideoConfig&    video()    const;
	const ModelConfig&    model()    const;
	const TrackingConfig& tracking() const;
	const LoggingConfig&  logging()  const;
	const DisplayConfig&  display()  const;

	// 输出当前生效的全部配置，用于启动时记录到日志、排查配置未生效的问题
	std::string describe() const;
};
