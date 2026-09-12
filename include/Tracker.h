#pragma	once

#include<opencv2/opencv.hpp>
#include<opencv2/tracking.hpp>

class Tracker
{
private:
	cv::Ptr<cv::Tracker> tracker;

public:
	Tracker();

	void init( 
		const cv::Mat& frame,
		const cv::Rect& bbox
	);

	bool update(
		const cv::Mat& frame,
		cv::Rect& bbox
	);
};
