#include <iostream>
#include<algorithm>
#include<cmath>
#include"MultiTracker.h"

MultiTracker::MultiTracker(const TrackingConfig& config)
	: next_id(0), config(config)
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

	if (union_area <= 0) return 0.0f;

	return static_cast<float>(intersection_area) / union_area;
}

std::vector<std::pair<int, int>> MultiTracker::hungarianMatch(
	const std::vector<std::vector<float>>& cost_matrix
)
{
	int n = static_cast<int>(cost_matrix.size());
	if (n == 0) return {};
	int m = static_cast<int>(cost_matrix[0].size());
	if (m == 0) return {};

	const float INF = 1e9f;
	int N = std::max(n, m);

	std::vector<std::vector<float>> a(N, std::vector<float>(N, INF));
	for (int i = 0; i < n; ++i)
		for (int j = 0; j < m; ++j)
			a[i][j] = cost_matrix[i][j];

	std::vector<float> u(N + 1, 0.0f), v(N + 1, 0.0f);
	std::vector<int> p(N + 1, 0), way(N + 1, 0);

	for (int i = 1; i <= N; i++)
	{
		p[0] = i;
		int j0 = 0;
		std::vector<float> minv(N + 1, INF);
		std::vector<bool> used(N + 1, false);

		do
		{
			used[j0] = true;
			int i0 = p[j0], j1 = 0;
			float delta = INF;

			for (int j = 1; j <= N; j++)
			{
				if (!used[j])
				{
					float curr = a[i0 - 1][j - 1] - u[i0] - v[j];
					if (curr < minv[j])
					{
						minv[j] = curr;
						way[j] = j0;
					}
					if (minv[j] < delta)
					{
						delta = minv[j];
						j1 = j;
					}
				}
			}

			for (int j = 0; j <= N; j++)
			{
				if (used[j])
				{
					u[p[j]] += delta;
					v[j] -= delta;
				}
				else
				{
					minv[j] -= delta;
				}
			}

			j0 = j1;

		} while (p[j0] != 0);

		do
		{
			int j1 = way[j0];
			p[j0] = p[j1];
			j0 = j1;
		} while (j0);

	}

	std::vector<std::pair<int, int>> matches;
	for (int j = 1; j <= N; ++j) {
		if (p[j] > 0 && p[j] <= n && j <= m) {
			if (a[p[j] - 1][j - 1] < INF / 2) {
				matches.push_back({ p[j] - 1, j - 1 });
			}
		}
	}
	return matches;

}

void MultiTracker::update(
	const std::vector<Detection>& detections,
	const cv::Mat& frame
)
{
	std::vector<cv::Rect> predicted_boxes;
	predicted_boxes.reserve(tracks.size());
	for (auto& track : tracks)
	{
		predicted_boxes.push_back(track.predict());
	}

	std::vector<std::vector<float>> cost_matrix(
		tracks.size(),
		std::vector<float>(detections.size(), 1.0f)
	);
	for (size_t i = 0; i < tracks.size(); ++i) {
		for (size_t j = 0; j < detections.size(); ++j)
		{
			if (detections[j].class_id != config.target_class_id) {
				cost_matrix[i][j] = 1e8f; // 非目标类，代价极大
				continue;
			}
			float iou = calculateIou(predicted_boxes[i], detections[j].box);
			cost_matrix[i][j] = 1.0f - iou;
		}
	}

	auto matches = hungarianMatch(cost_matrix);

	std::vector<bool> track_matched(tracks.size(), false);
	std::vector<bool> det_matched(detections.size(), false);

	for (auto& match : matches)
	{
		int t_idx = match.first;
		int d_idx = match.second;

		float iou = 1.0f - cost_matrix[t_idx][d_idx];
		if (iou < config.iou_threshold) continue; // 匹配度太低，放弃
		tracks[t_idx].updateKalman(detections[d_idx].box);

		// ★ 优化：只在 IoU 较低时重新初始化跟踪器，避免频繁 init 拖慢速度
		if (iou < config.reinit_iou_threshold)
		{
			tracks[t_idx].tracker->init(frame, detections[d_idx].box);
		}

		tracks[t_idx].lost_frame = 0;
		tracks[t_idx].active = true;
		track_matched[t_idx] = true;
		det_matched[d_idx] = true;
	}

	for (size_t i = 0; i < tracks.size(); i++)
	{
		if (!track_matched[i])
		{
			tracks[i].lost_frame++;
			if (tracks[i].lost_frame > config.max_lost_frames)
			{
				tracks[i].active = false;
			}
			else
			{
				if (tracks[i].tracker->update(frame, tracks[i].box))
				{
					tracks[i].updateKalman(tracks[i].box);
				}
			}
		}
	}

	for (size_t j = 0; j < detections.size(); j++)
	{
		if (!det_matched[j] && detections[j].class_id == config.target_class_id)
		{
			Track new_track(next_id++, detections[j].box, getRandomColor(), config);
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

		if (track.box.y < 30) label_y = track.box.y + 25;

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