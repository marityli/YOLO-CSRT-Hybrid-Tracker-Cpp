#include<yaml-cpp/yaml.h>
#include<iostream>

#include "Config.h"

Config::Config(const std::string& path)
{
	try
	{
		YAML::Node config = YAML::LoadFile(path);

		camera_id = config["camera"]["id"].as<int>();
		camera_width = config["camera"]["width"].as<int>();
		camera_height = config["camera"]["height"].as<int>();

		video_fps = config["video"]["fps"].as<double>();
		video_path = config["video"]["path"].as<std::string>();
	}
	catch (const YAML::Exception& e)
	{
		std::cerr << "Failed to load config file" << e.what() << std::endl;
		throw;
	}
		
}

int Config::getCameraId() const
{
	return camera_id;
}

int Config::getCameraWidth() const
{
	return camera_width;
}

int Config::getCameraHeight() const
{
	return camera_height;
}

double Config::getVideoFps() const
{
	return video_fps;
}

std::string Config::getVideoPath() const
{
	return video_path;
}

