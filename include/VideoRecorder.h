#pragma once

#include<opencv2/opencv.hpp>
#include<string>

class VideoRecorder
{
private:
	cv::VideoWriter writer;
	std::string filename;

public:
	VideoRecorder(
		const std::string& filename,
		double fps,
		cv::Size size,
		const std::string& fourcc = "mp4v"
	);

	bool isOpened() const;

	void write(const cv::Mat& frame);

	void release();
};
