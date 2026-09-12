#pragma once

#include <string>

class Config
{
private:
	int camera_id;
	int camera_width;
	int camera_height;

	double video_fps;
	std::string video_path;

public:
	explicit Config(const std::string& path);

	int getCameraId() const;
	int getCameraWidth() const;
	int getCameraHeight() const;

	double getVideoFps() const;
	std::string getVideoPath() const;
};