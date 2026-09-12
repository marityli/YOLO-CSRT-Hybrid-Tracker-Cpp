#pragma once

#include<chrono>

class FPS
{
private:
	std::chrono::steady_clock::time_point start_time;
	int frame_count;
	double fps;

public:

	FPS();

	double update();

};