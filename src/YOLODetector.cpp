#include<iostream>
#include<stdexcept>
#include<algorithm>
#include <chrono> 

#include"YOLODetector.h"

YOLODetector::YOLODetector(
	const std::string& model_path,
	float confidence_threshold,
	float nms_threshold,
	int input_width,
	int input_height,
	int intra_op_threads,
	bool use_cuda,
	int cuda_device_id,
	int profile_interval
)
	:env(ORT_LOGGING_LEVEL_WARNING, "YOLODetector"),
	confidence_threshold(confidence_threshold),
	nms_threshold(nms_threshold),
	input_width(input_width),
	input_height(input_height),
	intra_op_threads(intra_op_threads),
	use_cuda(use_cuda),
	cuda_device_id(cuda_device_id),
	profile_interval(profile_interval)
{
	// 0 表示交给 ONNX Runtime 按物理核心自动决定
	session_options.SetIntraOpNumThreads(intra_op_threads);
	session_options.SetGraphOptimizationLevel(
		GraphOptimizationLevel::ORT_ENABLE_ALL
	);

	if (use_cuda)
	{
		OrtCUDAProviderOptions cuda_options{};
		cuda_options.device_id = cuda_device_id;
		session_options.AppendExecutionProvider_CUDA(cuda_options);
	}
	else
	{
		std::cout << "CUDA provider disabled by config, running on CPU."
			<< std::endl;
	}

	try
	{
		std::wstring w_model_path(
			model_path.begin(),
			model_path.end()
		);

		session = Ort::Session(
			env,
			w_model_path.c_str(),
			session_options
		);

		Ort::AllocatorWithDefaultOptions allocator;

		auto providerS = Ort::GetAvailableProviders();
		std::cout << "===== Available Providers =====" << std::endl;
		for (const auto& provider : providerS) {
			std::cout << "- " << provider << std::endl;
		}
		std::cout << "================================" << std::endl;


		auto input_name_ptr = 
			session.GetInputNameAllocated(0, allocator);
		input_names_str.push_back(input_name_ptr.get());

		auto output_name_ptr =
			session.GetOutputNameAllocated(0, allocator);
		output_names_str.push_back(output_name_ptr.get());

		for (const auto& name : input_names_str)
		{
			input_names.push_back(name.c_str());
		}

		for (const auto& name : output_names_str) {
			output_names.push_back(name.c_str());
		}

	}
	catch (const Ort::Exception& e)
	{
		throw std::runtime_error(
			std::string("Failed to load ONNX model: ")
			+ e.what()
		);
	}
	std::cout << "YOLO ONNX model loaded successfully." << std::endl;
	std::cout << "  model: " << model_path
		<< " | input: " << input_width << "x" << input_height
		<< " | conf: " << confidence_threshold
		<< " | nms: " << nms_threshold
		<< " | intra_op_threads: " << intra_op_threads << std::endl;
}

std::vector<float> YOLODetector::preprocess(
	const cv::Mat& frame
)
{
	float scale = std::min(
		static_cast<float>(input_width) / frame.cols,
		static_cast<float>(input_height) / frame.rows
	);

	int new_w = static_cast<int>(frame.cols * scale);
	int new_h = static_cast<int>(frame.rows * scale);

	int pad_w = (input_width - new_w) / 2;
	int pad_h = (input_height - new_h) / 2;

	letterbox_scale = scale;
	letterbox_pad_x = pad_w;
	letterbox_pad_y = pad_h;

	cv::Mat resized;

	cv::resize(frame, resized, cv::Size(new_w, new_h));

	cv::Mat padded = cv::Mat::zeros(input_height, input_width, CV_8UC3);
	padded.setTo(cv::Scalar(114, 114, 114));
	resized.copyTo(padded(cv::Rect(pad_w, pad_h, new_w, new_h)));

	cv::cvtColor(padded, padded, cv::COLOR_BGR2RGB);

	padded.convertTo(padded, CV_32F, 1.0 / 255.0);

	std::vector<float> input_tensor;

	input_tensor.resize(3 * input_height * input_width);

	std::vector<cv::Mat> channels(3);

	for (int c = 0; c < 3; c++)
	{
		channels[c] = cv::Mat(
			input_height,
			input_width,
			CV_32F,
			input_tensor.data() + c * input_height * input_width
		);
	}

	cv::split(padded, channels);

	return input_tensor;
}

std::vector<Detection> YOLODetector::postprocess(
	const float* output,
	const std::vector<int64_t>& output_shape,
	const cv::Size& original_size
)
{

	std::vector<Detection> detections;

	if (output == nullptr)
	{
		std::cerr << "Error: output pointer is null" << std::endl;
		return detections;
	}

	if (output_shape.size() != 3)
	{
		std::cerr<< "Error: unsupported output rank: "
			<< output_shape.size()<< std::endl;
		return detections;
	}

	if (output_shape[0] != 1)
	{
		std::cerr<< "Error: only batch size 1 is supported."<< std::endl;
		return detections;
	}

	int64_t dim1 = output_shape[1];
	int64_t dim2 = output_shape[2];

	int64_t num_channels = 0;
	int64_t num_predictions = 0;
	bool channels_first = false;

	if (dim1 < dim2)
	{
		num_channels = dim1;
		num_predictions = dim2;
		channels_first = true;
	}
	else
	{
		num_predictions = dim1;
		num_channels = dim2;
		channels_first = false;
	}

	int64_t num_classes = num_channels - 4;

	if (num_classes <= 0)
	{
		std::cerr
			<< "Error: invalid number of classes: "
			<< num_classes
			<< std::endl;

		return detections;
	}

	/*
	std::cout
		<< "num_predictions: "<< num_predictions
		<< ", num_classes: "<< num_classes
		<< std::endl;
	*/
	std::vector<cv::Rect> boxes;
	std::vector<float> scores;
	std::vector<int> class_ids;
	

	//float x_factor = static_cast<float>(original_size.width) / input_width;
	//float y_factor = static_cast<float>(original_size.height) / input_height;

	for (int64_t i = 0; i < num_predictions; i++)
	{
		float cx, cy, w, h;

		if (channels_first)
		{
			cx = output[0 * num_predictions + i];
			cy = output[1 * num_predictions + i];
			w = output[2 * num_predictions + i];
			h = output[3 * num_predictions + i];
		}
		else
		{
			int64_t offset = i * num_channels;
			cx = output[offset + 0];
			cy = output[offset + 1];
			w = output[offset + 2];
			h = output[offset + 3];
		}

		float max_class_score = 0.0f;
		int class_id = -1;

		for (int64_t c = 0; c < num_classes; c++)
		{
			float class_score;
			if (channels_first)
			{
				class_score = output[(4 + c) * num_predictions + i];
			}
			else
			{
				int64_t offset = i * num_channels;
				class_score = output[offset + 4 + c];
			}

			if (class_score > max_class_score)
			{
				max_class_score = class_score;
				class_id = static_cast<int>(c);
			}
		}

		if (class_id < 0)
		{
			continue;
		}
		
		/*
		std::cout << "原始输出 -> cx: " << cx << " cy: " << cy
			<< " w: " << w << " h: " << h
			<< " score: " << max_class_score << std::endl;
			*/
		if (max_class_score < confidence_threshold)
		{
			continue;
		}

		float orig_cx = (cx - letterbox_pad_x) / letterbox_scale;
		float orig_cy = (cy - letterbox_pad_y) / letterbox_scale;
		float orig_w = w / letterbox_scale;
		float orig_h = h / letterbox_scale;

		int left = static_cast<int>(orig_cx - 0.5f * orig_w);
		int top = static_cast<int>(orig_cy - 0.5f * orig_h );
		int width = static_cast<int>(orig_w);
		int height = static_cast<int>(orig_h);

		cv::Rect box(left, top, width, height);

		box &= cv::Rect(
			0, 0, original_size.width, original_size.height
		);

		if (box.width <= 0 || box.height <= 0)
		{
			continue;
		}

		boxes.push_back(box);
		scores.push_back(max_class_score);
		class_ids.push_back(class_id);

	}

	std::vector<int> indices;

	cv::dnn::NMSBoxes(
		boxes,
		scores,
		confidence_threshold,
		nms_threshold,
		indices
	);

	for (int index : indices)
	{
		Detection detection;

		detection.class_id = class_ids[index];
		detection.confidence = scores[index];
		detection.box = boxes[index];

		detections.push_back(detection);
	}

	return detections;

}
/*
std::vector<Detection> YOLODetector::detect(
	const cv::Mat& frame
)
{
	std::vector<Detection> result;

	auto input_tensor_values = preprocess(frame);

	std::array<int64_t, 4> input_shape =
	{ 1,3,input_height,input_width };

	auto memory_info =
		Ort::MemoryInfo::CreateCpu(
			OrtArenaAllocator,
			OrtMemTypeDefault
		);

	Ort::Value input_tensor =
		Ort::Value::CreateTensor<float>(
			memory_info,
			input_tensor_values.data(),
			input_tensor_values.size(),
			input_shape.data(),
			input_shape.size()
		);

	auto output_tensor =
		session.Run(
			Ort::RunOptions{ nullptr },
			input_names.data(),
			&input_tensor,
			1,
			output_names.data(),
			1
		);

	auto output_info = output_tensor[0].GetTensorTypeAndShapeInfo();

	std::vector<int64_t> output_shape = output_info.GetShape();

	float* output = output_tensor[0].GetTensorMutableData<float>();

	result = postprocess(output, output_shape, frame.size());

	return result;
}
*/

std::vector<Detection> YOLODetector::detect(
	const cv::Mat& frame
)
{
	std::vector<Detection> result;

	// 1. 预处理计时
	auto start_pre = std::chrono::high_resolution_clock::now();
	auto input_tensor_values = preprocess(frame);
	auto end_pre = std::chrono::high_resolution_clock::now();

	// 2. 创建张量
	std::array<int64_t, 4> input_shape = { 1, 3, input_height, input_width };
	auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPUInput);
	Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
		memory_info,
		input_tensor_values.data(),
		input_tensor_values.size(),
		input_shape.data(),
		input_shape.size()
	);

	// 3. 推理计时
	auto start_run = std::chrono::high_resolution_clock::now();
	auto output_tensor = session.Run(
		Ort::RunOptions{ nullptr },
		input_names.data(),
		&input_tensor,
		1,
		output_names.data(),
		1
	);
	auto end_run = std::chrono::high_resolution_clock::now();

	// 4. 后处理计时
	auto output_info = output_tensor[0].GetTensorTypeAndShapeInfo();
	std::vector<int64_t> output_shape = output_info.GetShape();
	float* output = output_tensor[0].GetTensorMutableData<float>();

	auto start_post = std::chrono::high_resolution_clock::now();
	result = postprocess(output, output_shape, frame.size());
	auto end_post = std::chrono::high_resolution_clock::now();

	// 5. 计算并打印耗时（每 30 帧打印一次）
	auto pre_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_pre - start_pre).count();
	auto run_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_run - start_run).count();
	auto post_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_post - start_post).count();

	static int frame_count = 0;
	if (profile_interval > 0 && frame_count++ % profile_interval == 0) {
		std::cout << "耗时统计 -> 预处理: " << pre_ms << "ms | 推理(GPU): " << run_ms
			<< "ms | 后处理: " << post_ms << "ms" << std::endl;
	}

	return result;
}