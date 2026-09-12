#pragma once

#include<string>

class Logger
{
private:
	std::string log_file;

	void write(
		const std::string& level,
		const std::string& message
	);

public:
	explicit Logger(const std::string& log_gile);

	void info(const std::string& message);
	void warning(const std::string& message);
	void error(const std::string& message);
};