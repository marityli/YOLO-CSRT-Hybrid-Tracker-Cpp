#include"VideoRecorder.h"

#include<iostream>
#include<filesystem>

VideoRecorder::VideoRecorder(
	const std::string& filename,
	double fps,
	cv::Size size,
	const std::string& fourcc
)
	: filename(filename)
{
	if (size.width <= 0 || size.height <= 0 || fps <= 0.0)
	{
		std::cerr << "Invalid video recorder parameters: size="
			<< size.width << "x" << size.height
			<< " fps=" << fps << std::endl;
		return;
	}

	// 输出目录不存在时自动创建
	std::error_code error;
	std::filesystem::path path(filename);

	if (path.has_parent_path())
	{
		std::filesystem::create_directories(path.parent_path(), error);

		if (error)
		{
			std::cerr << "Failed to create video directory '"
				<< path.parent_path().string() << "': "
				<< error.message() << std::endl;
		}
	}

	std::string code = fourcc;

	// fourcc 必须是 4 个字符，否则退回 mp4v
	if (code.size() != 4)
	{
		std::cerr << "Invalid fourcc '" << fourcc
			<< "', falling back to mp4v" << std::endl;
		code = "mp4v";
	}

	int fourcc_code = cv::VideoWriter::fourcc(
		code[0], code[1], code[2], code[3]
	);

	writer.open(filename, fourcc_code, fps, size);

	if (!writer.isOpened())
	{
		std::cerr << "Failed to open video writer! path=" << filename
			<< " fourcc=" << code
			<< " fps=" << fps
			<< " size=" << size.width << "x" << size.height << std::endl;
	}
	else
	{
		std::cout << "Video writer opened: " << filename
			<< " (" << code << ", " << fps << " fps, "
			<< size.width << "x" << size.height << ")" << std::endl;
	}
}

bool VideoRecorder::isOpened() const
{
	return writer.isOpened();
}

void VideoRecorder::write(const cv::Mat& frame)
{
	if (writer.isOpened())
	{
		writer.write(frame);
	}
}

void VideoRecorder::release()
{
	writer.release();
}
