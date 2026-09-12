#include<fstream>
#include<iostream>
#include<sstream>
#include<chrono>
#include<iomanip>

#include"Logger.h"



Logger::Logger(const std::string& log_file)
	: log_file(log_file)
{
}

void Logger::write(
	const std::string& level,
	const std::string& message)
{
	auto now = std::chrono::system_clock::now();
	auto time = std::chrono::system_clock::to_time_t(now);

	std::tm local_time{};

#ifdef _WIN32
	localtime_s(&local_time, &time);
#else
	localtime_r(&time, &local_time);
#endif // _WIN32

	std::ostringstream timestamp;

	timestamp << std::put_time(
		&local_time,
		"%Y-%m-%d %H:%M:%S"
	);

	std::string output =
		timestamp.str()
		+ " [" + level + "] "
		+ message;

	std::cout << output << std::endl;

	std::ofstream file(log_file, std::ios::app);

	if (file.is_open())
	{
		file << output << std::endl;
	}
}

void Logger::info(const std::string& message)
{
	write("INFO", message);
}
void Logger::warning(const std::string& message)
{
	write("WARNING", message);
}
void Logger::error(const std::string& message)
{
	write("ERROR", message);
}