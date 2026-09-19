#pragma once

#include<opencv2/opencv.hpp>
#include<onnxruntime_cxx_api.h>

#include<string>
#include<vector>

struct Detection
{
	int class_id;
	float confidence;
	cv::Rect box;
};

class YOLODetector
{
private:
	Ort::Env env;
	Ort::SessionOptions session_options;
	Ort::Session session{nullptr};

	std::vector<std::string> input_names_str;
	std::vector<std::string> output_names_str;

	std::vector<const char*> input_names;
	std::vector<const char*> output_names;

	float confidence_threshold;
	float nms_threshold;

	int input_width;
	int input_height;

	int intra_op_threads;
	bool use_cuda;
	int cuda_device_id;
	int profile_interval;

	float letterbox_scale = 1.0f;
	int letterbox_pad_x = 0;
	int letterbox_pad_y = 0;

	std::vector<float> preprocess(const cv::Mat& frame);

	std::vector<Detection> postprocess(
		const float* output,
		const std::vector<int64_t>& output_shape,
		const cv::Size& original_size
	);

public:
	YOLODetector(
		const std::string& model_path,
		float confidence_threshold = 0.5f,
		float nms_threshold = 0.45f,
		int input_width = 640,
		int input_height = 640,
		int intra_op_threads = 0,
		bool use_cuda = true,
		int cuda_device_id = 0,
		int profile_interval = 30
	);

	std::vector<Detection> detect(const cv::Mat& frame);

};
