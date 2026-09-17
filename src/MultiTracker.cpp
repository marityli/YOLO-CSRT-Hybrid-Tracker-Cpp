#include <iostream>

#include"MultiTracker.h"

MultiTracker::MultiTracker()
{
}

cv::Scalar MultiTracker::getRandomColor()
{
	return cv::Scalar(
		rand() % 256,
		rand() % 256,
		rand() % 256
	);
}

float MultiTracker::calculateIou(
	const cv::Rect& box1,
	const cv::Rect& box2
)
{
	int x1 = std::max(box1.x, box2.x);
	int y1 = std::max(box1.y, box2.y);
	int x2 = std::min(box1.x + box1.width, box2.x + box2.width);
	int y2 = std::min(box1.y + box1.height, box2.y + box2.height);

	int intersection_area = std::max(0, x2 - x1) * std::max(0, y2 - y1);
	int union_area = box1.area() + box2.area() - intersection_area;

	return static_cast<float>(intersection_area) / union_area;
}

void MultiTracker::update(
	const std::vector<Detection>& detections,
	const cv::Mat& frame
)
{
	std::vector<bool> track_matched(tracks.size(), false);
	std::vector<bool> det_matched(detections.size(), false);


	for (size_t i = 0; i < tracks.size(); i++)
	{
		float max_iou = 0.0f;
		int best_det_idx = -1;

		for (size_t j = 0; j < detections.size(); j++)
		{
			if (det_matched[j]) continue;
			if (detections[j].class_id != 0) continue;

			float iou = calculateIou(tracks[i].box, detections[j].box);
			if (iou > max_iou && iou > iou_threshold)
			{
				max_iou = iou;
				best_det_idx = static_cast<int>(j);
			}
		}

		if (best_det_idx != -1)
		{
			tracks[i].box = detections[best_det_idx].box;
			tracks[i].tracker->init(frame, tracks[i].box);
			tracks[i].lost_frame = 0;
			tracks[i].active = true;

			track_matched[i] = true;
			det_matched[best_det_idx] = true;
		}
	}
	for (size_t i = 0; i < tracks.size(); i++)
	{
		if (!track_matched[i])
		{
			tracks[i].lost_frame++;
			if (tracks[i].lost_frame > max_lost_frames)
			{
				tracks[i].active = false;
			}
			else
			{
				tracks[i].tracker->update(frame, tracks[i].box);
			}
		}
	}

	for (size_t j = 0; j < detections.size(); j++)
	{
		if (!det_matched[j] && detections[j].class_id == 0)
		{
			Track new_track;
			new_track.track_id = next_id++;
			new_track.box = detections[j].box;
			new_track.lost_frame = 0;
			new_track.active = true;
			new_track.color = getRandomColor();
			new_track.tracker = std::make_unique<Tracker>();
			new_track.tracker->init(frame, new_track.box);

			tracks.push_back(std::move(new_track));
		}
	}

	tracks.erase(
		std::remove_if(tracks.begin(), tracks.end(),
			[](const Track& t) {return !t.active; }),
		tracks.end()
	);

}

void MultiTracker::draw(cv::Mat& frame)
{
	for (const auto& track : tracks)
	{
		cv::rectangle(frame, track.box, track.color, 2);

		std::string label = "ID: " + std::to_string(track.track_id);
		int label_y = std::max(0, track.box.y - 10);

		cv::putText(
			frame,
			label,
			cv::Point(track.box.x, label_y),
			cv::FONT_HERSHEY_SIMPLEX,
			0.7,
			track.color,
			2
		);
	}
}