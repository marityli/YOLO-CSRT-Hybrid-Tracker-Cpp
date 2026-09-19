#include<iostream>

#include"Tracker.h"

Tracker::Tracker()
{
	tracker = nullptr;
}

void Tracker::init(
	const cv::Mat& frame,
	const cv::Rect& bbox
)
{
	tracker = cv::TrackerKCF::create();

	tracker->init(frame, bbox);
}

bool Tracker::update(
	const cv::Mat& frame,
	cv::Rect& bbox
)
{
	if (tracker == nullptr)
	{
		return false;
	}
	return tracker->update(frame, bbox);
}