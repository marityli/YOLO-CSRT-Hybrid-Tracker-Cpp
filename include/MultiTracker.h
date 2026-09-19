#pragma once

#include<vector>
#include<memory>
#include <climits>
#include<opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>
#include"Tracker.h"
#include"YOLODetector.h"
#include"Config.h"

struct Track
{
	int track_id;
	cv::Rect box;
	int lost_frame;
	std::unique_ptr<Tracker> tracker;
	bool active;
	cv::Scalar color;

	cv::KalmanFilter kf;
	cv::Mat prediction;

	Track()
		: track_id(-1), lost_frame(0), active(false), color(0, 0, 0)
	{}

	// config 用来取 Kalman 的噪声参数，默认参数保证旧调用方式依然可用
	Track(int id, const cv::Rect& initial_box, cv::Scalar c,
		const TrackingConfig& config = TrackingConfig())
		:track_id(id), box(initial_box), lost_frame(0),
		active(true), color(c)
	{
		kf.init(8, 4, 0);
		kf.transitionMatrix = (cv::Mat_<float>(8, 8) <<
			1, 0, 0, 0, 1, 0, 0, 0,
			0, 1, 0, 0, 0, 1, 0, 0,
			0, 0, 1, 0, 0, 0, 1, 0,
			0, 0, 0, 1, 0, 0, 0, 1,
			0, 0, 0, 0, 1, 0, 0, 0,
			0, 0, 0, 0, 0, 1, 0, 0,
			0, 0, 0, 0, 0, 0, 1, 0,
			0, 0, 0, 0, 0, 0, 0, 1);
		cv::setIdentity(kf.measurementMatrix);
		cv::setIdentity(kf.processNoiseCov, cv::Scalar::all(config.kalman.process_noise));
		cv::setIdentity(kf.measurementNoiseCov, cv::Scalar::all(config.kalman.measurement_noise));
		cv::setIdentity(kf.errorCovPost, cv::Scalar::all(config.kalman.error_cov_post));

		float cx = initial_box.x + initial_box.width / 2.0f;
		float cy = initial_box.y + initial_box.height / 2.0f;
		float s = static_cast<float>(initial_box.width * initial_box.height);
		float r = static_cast<float>(initial_box.width) / initial_box.height;
		kf.statePost = (cv::Mat_<float>(8, 1) << cx, cy, s, r, 0, 0, 0, 0);
	}

	cv::Rect predict()
	{
		prediction = kf.predict();
		float cx = prediction.at<float>(0);
		float cy = prediction.at<float>(1);
		float s = prediction.at<float>(2);
		float r = prediction.at<float>(3);

		float w = std::sqrt(std::max(s * r, 1.0f));
		float h = std::max(s / w, 1.0f);

		return cv::Rect(
			static_cast<int>(cx - w / 2),
			static_cast<int>(cy - h / 2),
			static_cast<int>(w),
			static_cast<int>(h)
		);
	}

	void updateKalman(const cv::Rect& measured_box)
	{
		float cx = measured_box.x + measured_box.width / 2.0f;
		float cy = measured_box.y + measured_box.height / 2.0f;
		float s = static_cast<float>(measured_box.width * measured_box.height);
		float r = static_cast<float>(measured_box.width) / measured_box.height;
		cv::Mat measurement = (cv::Mat_<float>(4, 1) << cx, cy, s, r);

		kf.correct(measurement);
		this->box = measured_box;
	}

};

class MultiTracker
{
private:
	std::vector<Track> tracks;
	int next_id = 0;

	TrackingConfig config;

	float calculateIou(
		const cv::Rect& box1,
		const cv::Rect& box2
	);

	cv::Scalar getRandomColor();

	std::vector<std::pair<int, int>> hungarianMatch(
		const std::vector<std::vector<float>>& cost_matrix
	);

public:
	explicit MultiTracker(const TrackingConfig& config = TrackingConfig());

	void update(
		const std::vector<Detection>& detections,
		const cv::Mat& frame
	);

	void draw(cv::Mat& frame);

};
