#pragma once

#include<string>
#include<fstream>

class Logger
{
public:
	enum class Level
	{
		DEBUG = 0,
		INFO = 1,
		WARNING = 2,
		ERROR = 3
	};

private:
	std::string log_file;
	std::ofstream file;

	Level min_level;
	bool console;

	void write(
		Level level,
		const std::string& message
	);

	static Level parseLevel(const std::string& name);
	static const char* levelName(Level level);

public:
	explicit Logger(
		const std::string& log_file,
		const std::string& level = "INFO",
		bool console = true
	);

	~Logger();

	void debug(const std::string& message);
	void info(const std::string& message);
	void warning(const std::string& message);
	void error(const std::string& message);
};
