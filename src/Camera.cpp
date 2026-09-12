#include "Camera.h"
#include <iostream>

Camera::Camera(
	int camera_id,
	int width,
	int height)
{
	cap.open(camera_id);

	if (!cap.isOpened())
	{
		std::cerr << "Failed to open camera" << std::endl;
		return;
	}

	cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
	cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);

}

cv::Mat Camera::read()
{
	cv::Mat frame;

	cap >> frame;

	return frame;
}

void Camera::release()
{
	cap.release();
}