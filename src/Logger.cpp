#include<fstream>
#include<iostream>
#include<sstream>
#include<chrono>
#include<iomanip>
#include<filesystem>

#include"Logger.h"

Logger::Logger(
	const std::string& log_file,
	const std::string& level,
	bool console
)
	: log_file(log_file),
	min_level(parseLevel(level)),
	console(console)
{
	// 日志目录不存在时自动创建，避免首次运行写不进日志
	std::error_code error;
	std::filesystem::path path(log_file);

	if (path.has_parent_path())
	{
		std::filesystem::create_directories(path.parent_path(), error);

		if (error)
		{
			std::cerr << "Failed to create log directory '"
				<< path.parent_path().string() << "': "
				<< error.message() << std::endl;
		}
	}

	file.open(log_file, std::ios::app);

	if (!file.is_open())
	{
		std::cerr << "Failed to open log file: " << log_file << std::endl;
	}
}

Logger::~Logger()
{
	if (file.is_open())
	{
		file.close();
	}
}

Logger::Level Logger::parseLevel(const std::string& name)
{
	if (name == "DEBUG")   return Level::DEBUG;
	if (name == "INFO")    return Level::INFO;
	if (name == "WARNING") return Level::WARNING;
	if (name == "ERROR")   return Level::ERROR;

	std::cerr << "Unknown log level '" << name
		<< "', falling back to INFO" << std::endl;

	return Level::INFO;
}

const char* Logger::levelName(Level level)
{
	switch (level)
	{
	case Level::DEBUG:   return "DEBUG";
	case Level::INFO:    return "INFO";
	case Level::WARNING: return "WARNING";
	case Level::ERROR:   return "ERROR";
	}

	return "INFO";
}

void Logger::write(
	Level level,
	const std::string& message)
{
	if (level < min_level)
	{
		return;
	}

	auto now = std::chrono::system_clock::now();
	auto time = std::chrono::system_clock::to_time_t(now);

	// 毫秒对定位"单帧卡住导致窗口无响应"这类问题很关键
	auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
		now.time_since_epoch()) % 1000;

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
	)
		<< '.'
		<< std::setfill('0') << std::setw(3) << millis.count();

	std::string output =
		timestamp.str()
		+ " [" + levelName(level) + "] "
		+ message;

	if (console)
	{
		std::cout << output << std::endl;
	}

	if (file.is_open())
	{
		file << output << std::endl;
		file.flush();
	}
}

void Logger::debug(const std::string& message)
{
	write(Level::DEBUG, message);
}

void Logger::info(const std::string& message)
{
	write(Level::INFO, message);
}

void Logger::warning(const std::string& message)
{
	write(Level::WARNING, message);
}

void Logger::error(const std::string& message)
{
	write(Level::ERROR, message);
}
