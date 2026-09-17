#pragma once

#include<vector>
#include<memory>
#include<opencv2/opencv.hpp>

#include"Tracker.h"
#include"YOLODetector.h"

struct Track
{
	int track_id;
	cv::Rect box;
	int lost_frame;
	std::unique_ptr<Tracker> tracker;
	bool active;
	cv::Scalar color;
};

class MultiTracker
{
private:
	std::vector<Track> tracks;
	int next_id;

	const int max_lost_frames = 30;
	const float iou_threshold = 0.3f;

	float calculateIou(
		const cv::Rect& box1,
		const cv::Rect& box2
	);

	cv::Scalar getRandomColor();

public:
	MultiTracker();

	void update(
		const std::vector<Detection>& detections,
		const cv::Mat& frame
	);

	void draw(cv::Mat& frame);

};