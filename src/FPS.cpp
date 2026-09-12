#include "FPS.h"

FPS::FPS()
{
	start_time = std::chrono::steady_clock::now();
	frame_count = 0;
	fps = 0.0;
}

double FPS::update()
{
	frame_count++;
	auto curr_time = std::chrono::steady_clock::now();

	std::chrono::duration<double> elapsed = curr_time - start_time;

	if (elapsed.count() >= 1.0)
	{
		fps = frame_count / elapsed.count();

		frame_count = 0;
		start_time = curr_time;
	}

	return fps;
}