#include"VideoRecorder.h"

#include<iostream>

VideoRecorder::VideoRecorder(
	const std::string& filename,
	double fps,
	cv::Size size
)
{
	int fourcc = cv::VideoWriter::fourcc(
		'm', 'p', '4', 'v'
	);

	writer.open(filename, fourcc, fps, size);

	if (!writer.isOpened())
	{
		std::cerr << "Failed to open video writer!" << std::endl;
	}
}

void VideoRecorder::write(const cv::Mat& frame)
{
	if (writer.isOpened())
	{
		writer.write(frame);
	}
}

void VideoRecorder::release()
{
	writer.release();
}