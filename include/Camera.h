#pragma once

#include <opencv2/opencv.hpp>

class Camera
{
private:
	cv::VideoCapture cap;

public:

	explicit Camera(
		int camera_id,
		int width,
		int height
	);

	cv::Mat read();
	
	void release();



};