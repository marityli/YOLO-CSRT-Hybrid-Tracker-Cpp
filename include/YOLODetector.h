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
		float nms_threshold = 0.45f
	);

	std::vector<Detection> detect(const cv::Mat& frame);

};