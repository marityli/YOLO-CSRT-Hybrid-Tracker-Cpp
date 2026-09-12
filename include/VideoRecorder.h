#pragma once

#include<opencv2/opencv.hpp>
#include<string>

class VideoRecorder
{
private:
	cv::VideoWriter writer;

public:
	VideoRecorder(
		const std::string& filename,
		double fps,
		cv::Size size
	);

	void write(const cv::Mat& frame);

	void release();
};