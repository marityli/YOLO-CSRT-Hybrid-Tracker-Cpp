#include<yaml-cpp/yaml.h>
#include<iostream>
#include<sstream>
#include<string>
#include<vector>
#include<stdexcept>
#include<cmath>
#include<cctype>
#include<utility>
#include<deque>

#include "Config.h"

namespace
{
	// 只读查找一个键：遍历 map 而不用 operator[]。
	//
	// 为什么不直接用 node[key]：
	//   1. 非 const 的 operator[] 在键不存在时会【插入】这个键；
	//   2. 更隐蔽的是 Node::operator= 对已定义的节点是"就地写入"（set）而不是
	//      重新绑定指针，所以 `current = current[key]` 会把子节点内容覆盖到
	//      当前节点上，直接污染整棵配置树 —— 表现为第一次取值成功、
	//      之后所有键都被误判为"缺失"。
	//   用迭代器遍历可以完全避开这两个坑。
	YAML::Node lookupKey(const YAML::Node& map, const std::string& key)
	{
		if (!map.IsMap())
		{
			return YAML::Node();
		}

		for (auto it = map.begin(); it != map.end(); ++it)
		{
			if (!it->first.IsScalar())
			{
				continue;
			}

			try
			{
				if (it->first.as<std::string>() == key)
				{
					return it->second;
				}
			}
			catch (const YAML::Exception&)
			{
				continue;
			}
		}

		return YAML::Node();
	}

	// 按 "a.b.c" 的形式逐层取节点；任一层缺失则返回未定义节点
	YAML::Node nodeAt(const YAML::Node& root, const std::string& path)
	{
		// 用 deque 保存每一层的副本：push_back 不会让已有元素的引用失效，
		// 因此 back() 始终保持有效，同时又能保证节点生命周期长于本次调用。
		std::deque<YAML::Node> chain;
		chain.push_back(root);

		std::istringstream stream(path);
		std::string key;

		while (std::getline(stream, key, '.'))
		{
			YAML::Node child = lookupKey(chain.back(), key);

			if (!child || !child.IsDefined())
			{
				return YAML::Node();
			}

			chain.push_back(child);
		}

		return chain.back();
	}

	// 取值：缺失记入 missing，类型不对记入 invalid，两种情况都回退到 fallback
	template <typename T>
	T readValue(
		const YAML::Node& root,
		const std::string& path,
		const T& fallback,
		std::vector<std::string>& missing,
		std::vector<std::string>& invalid)
	{
		YAML::Node node = nodeAt(root, path);

		if (!node || !node.IsDefined() || node.IsNull())
		{
			missing.push_back(path);
			return fallback;
		}

		try
		{
			return node.as<T>();
		}
		catch (const YAML::Exception&)
		{
			invalid.push_back(path);
			return fallback;
		}
	}

	// 把 "a=1, b=2" 这种修正记录收集起来统一打印
	struct Correction
	{
		std::string key;
		std::string reason;
	};

	std::vector<Correction>& corrections()
	{
		static std::vector<Correction> list;
		return list;
	}

	void fix(std::string key, std::string reason)
	{
		corrections().push_back({ std::move(key), std::move(reason) });
	}
}

Config::Config(const std::string& path)
{
	YAML::Node root;

	try
	{
		root = YAML::LoadFile(path);
	}
	catch (const YAML::Exception& e)
	{
		std::cerr << "[Config] Failed to load config file '" << path
			<< "': " << e.what() << std::endl;
		throw;
	}

	if (!root.IsMap())
	{
		std::cerr << "[Config] Config root must be a mapping (key: value), got something else."
			<< std::endl;
		throw std::runtime_error("Config root node is not a map");
	}

	std::vector<std::string> missing;
	std::vector<std::string> invalid;

	// 匿名函数：按 "分组.键" 取默认值
	auto getInt = [&](const std::string& key, int fallback)
		{ return readValue<int>(root, key, fallback, missing, invalid); };
	auto getFloat = [&](const std::string& key, float fallback)
		{ return readValue<float>(root, key, fallback, missing, invalid); };
	auto getDouble = [&](const std::string& key, double fallback)
		{ return readValue<double>(root, key, fallback, missing, invalid); };
	auto getBool = [&](const std::string& key, bool fallback)
		{ return readValue<bool>(root, key, fallback, missing, invalid); };
	auto getString = [&](const std::string& key, const std::string& fallback)
		{ return readValue<std::string>(root, key, fallback, missing, invalid); };

	// ---------- 相机 ----------
	camera_config.id     = getInt("camera.id", camera_config.id);
	camera_config.width  = getInt("camera.width", camera_config.width);
	camera_config.height = getInt("camera.height", camera_config.height);

	// ---------- 视频 ----------
	video_config.path   = getString("video.path", video_config.path);
	video_config.fps    = getDouble("video.fps", video_config.fps);
	video_config.fourcc = getString("video.fourcc", video_config.fourcc);

	// ---------- 模型 ----------
	model_config.path = getString("model.path", model_config.path);
	model_config.confidence_threshold =
		getFloat("model.confidence_threshold", model_config.confidence_threshold);
	model_config.nms_threshold =
		getFloat("model.nms_threshold", model_config.nms_threshold);
	model_config.input_width =
		getInt("model.input_width", model_config.input_width);
	model_config.input_height =
		getInt("model.input_height", model_config.input_height);
	model_config.intra_op_threads =
		getInt("model.intra_op_threads", model_config.intra_op_threads);
	model_config.use_cuda =
		getBool("model.use_cuda", model_config.use_cuda);
	model_config.cuda_device_id =
		getInt("model.cuda_device_id", model_config.cuda_device_id);
	model_config.profile_interval =
		getInt("model.profile_interval", model_config.profile_interval);

	// ---------- 跟踪 ----------
	tracking_config.detection_interval =
		getInt("tracking.detection_interval", tracking_config.detection_interval);
	tracking_config.target_class_id =
		getInt("tracking.target_class_id", tracking_config.target_class_id);
	tracking_config.iou_threshold =
		getFloat("tracking.iou_threshold", tracking_config.iou_threshold);
	tracking_config.reinit_iou_threshold =
		getFloat("tracking.reinit_iou_threshold", tracking_config.reinit_iou_threshold);
	tracking_config.max_lost_frames =
		getInt("tracking.max_lost_frames", tracking_config.max_lost_frames);

	tracking_config.kalman.process_noise =
		getFloat("tracking.kalman.process_noise", tracking_config.kalman.process_noise);
	tracking_config.kalman.measurement_noise =
		getFloat("tracking.kalman.measurement_noise", tracking_config.kalman.measurement_noise);
	tracking_config.kalman.error_cov_post =
		getFloat("tracking.kalman.error_cov_post", tracking_config.kalman.error_cov_post);

	// ---------- 日志 ----------
	logging_config.path    = getString("logging.path", logging_config.path);
	logging_config.level   = getString("logging.level", logging_config.level);
	logging_config.console = getBool("logging.console", logging_config.console);

	// ---------- 显示 ----------
	display_config.window_name = getString("display.window_name", display_config.window_name);
	display_config.show_window = getBool("display.show_window", display_config.show_window);
	display_config.draw_fps    = getBool("display.draw_fps", display_config.draw_fps);
	display_config.profile_interval =
		getInt("display.profile_interval", display_config.profile_interval);

	// ---------- 缺键 / 类型错误 / 取值越界 提示 ----------
	if (!missing.empty())
	{
		std::cout << "[Config] " << missing.size()
			<< " key(s) missing in '" << path << "', using built-in defaults:";

		for (const auto& key : missing)
		{
			std::cout << ' ' << key;
		}

		std::cout << std::endl;
	}

	if (!invalid.empty())
	{
		std::cerr << "[Config] " << invalid.size()
			<< " key(s) have invalid type and were ignored:";

		for (const auto& key : invalid)
		{
			std::cerr << ' ' << key;
		}

		std::cerr << std::endl;
	}

	validate();

	for (const auto& item : corrections())
	{
		std::cerr << "[Config] Corrected '" << item.key << "': "
			<< item.reason << std::endl;
	}

	corrections().clear();
}

// 把明显不合法的取值拉回可用范围，避免运行到一半才崩
void Config::validate()
{
	// 相机
	if (camera_config.width <= 0)
	{
		fix("camera.width", "must be > 0, reset to 640");
		camera_config.width = 640;
	}

	if (camera_config.height <= 0)
	{
		fix("camera.height", "must be > 0, reset to 480");
		camera_config.height = 480;
	}

	if (camera_config.id < 0)
	{
		fix("camera.id", "must be >= 0, reset to 0");
		camera_config.id = 0;
	}

	// 视频
	if (video_config.fps <= 0.0)
	{
		fix("video.fps", "must be > 0, reset to 30");
		video_config.fps = 30.0;
	}

	if (video_config.fourcc.size() != 4)
	{
		fix("video.fourcc", "must be exactly 4 characters, reset to mp4v");
		video_config.fourcc = "mp4v";
	}

	// 模型
	if (model_config.input_width <= 0 || model_config.input_height <= 0)
	{
		fix("model.input_width/height", "must be > 0, reset to 640x640");
		model_config.input_width = 640;
		model_config.input_height = 640;
	}

	if (model_config.input_width % 32 != 0 || model_config.input_height % 32 != 0)
	{
		fix("model.input_width/height",
			"YOLO requires multiples of 32, current value may fail at inference");
	}

	if (model_config.confidence_threshold <= 0.0f
		|| model_config.confidence_threshold > 1.0f)
	{
		fix("model.confidence_threshold", "must be in (0, 1], reset to 0.5");
		model_config.confidence_threshold = 0.5f;
	}

	if (model_config.nms_threshold <= 0.0f
		|| model_config.nms_threshold > 1.0f)
	{
		fix("model.nms_threshold", "must be in (0, 1], reset to 0.45");
		model_config.nms_threshold = 0.45f;
	}

	if (model_config.intra_op_threads < 0)
	{
		fix("model.intra_op_threads", "must be >= 0 (0 = auto), reset to 0");
		model_config.intra_op_threads = 0;
	}

	if (model_config.cuda_device_id < 0)
	{
		fix("model.cuda_device_id", "must be >= 0, reset to 0");
		model_config.cuda_device_id = 0;
	}

	if (model_config.profile_interval < 1)
	{
		fix("model.profile_interval", "must be >= 1, reset to 30");
		model_config.profile_interval = 30;
	}

	// 跟踪
	if (tracking_config.detection_interval < 1)
	{
		fix("tracking.detection_interval", "must be >= 1, reset to 15");
		tracking_config.detection_interval = 15;
	}

	if (tracking_config.target_class_id < 0)
	{
		fix("tracking.target_class_id", "must be >= 0, reset to 0");
		tracking_config.target_class_id = 0;
	}

	if (tracking_config.iou_threshold < 0.0f
		|| tracking_config.iou_threshold > 1.0f)
	{
		fix("tracking.iou_threshold", "must be in [0, 1], reset to 0.3");
		tracking_config.iou_threshold = 0.3f;
	}

	if (tracking_config.reinit_iou_threshold < 0.0f
		|| tracking_config.reinit_iou_threshold > 1.0f)
	{
		fix("tracking.reinit_iou_threshold", "must be in [0, 1], reset to 0.6");
		tracking_config.reinit_iou_threshold = 0.6f;
	}

	if (tracking_config.max_lost_frames < 0)
	{
		fix("tracking.max_lost_frames", "must be >= 0, reset to 30");
		tracking_config.max_lost_frames = 30;
	}

	if (tracking_config.kalman.process_noise <= 0.0f)
	{
		fix("tracking.kalman.process_noise", "must be > 0, reset to 0.01");
		tracking_config.kalman.process_noise = 1e-2f;
	}

	if (tracking_config.kalman.measurement_noise <= 0.0f)
	{
		fix("tracking.kalman.measurement_noise", "must be > 0, reset to 0.1");
		tracking_config.kalman.measurement_noise = 1e-1f;
	}

	if (tracking_config.kalman.error_cov_post <= 0.0f)
	{
		fix("tracking.kalman.error_cov_post", "must be > 0, reset to 1.0");
		tracking_config.kalman.error_cov_post = 1.0f;
	}

	// 日志：级别统一成大写并校验
	for (auto& ch : logging_config.level)
	{
		ch = static_cast<char>(::toupper(static_cast<unsigned char>(ch)));
	}

	if (logging_config.level != "DEBUG"
		&& logging_config.level != "INFO"
		&& logging_config.level != "WARNING"
		&& logging_config.level != "ERROR")
	{
		fix("logging.level",
			"unknown level '" + logging_config.level + "', reset to INFO");
		logging_config.level = "INFO";
	}

	// 显示
	if (display_config.profile_interval < 1)
	{
		fix("display.profile_interval", "must be >= 1, reset to 10");
		display_config.profile_interval = 10;
	}
}

const CameraConfig& Config::camera() const
{
	return camera_config;
}

const VideoConfig& Config::video() const
{
	return video_config;
}

const ModelConfig& Config::model() const
{
	return model_config;
}

const TrackingConfig& Config::tracking() const
{
	return tracking_config;
}

const LoggingConfig& Config::logging() const
{
	return logging_config;
}

const DisplayConfig& Config::display() const
{
	return display_config;
}

std::string Config::describe() const
{
	std::ostringstream out;

	out << "----- effective config -----\n";

	out << "[camera]   id=" << camera_config.id
		<< " size=" << camera_config.width << "x" << camera_config.height << "\n";

	out << "[video]    path=" << video_config.path
		<< " fps=" << video_config.fps
		<< " fourcc=" << video_config.fourcc << "\n";

	out << "[model]    path=" << model_config.path
		<< " conf=" << model_config.confidence_threshold
		<< " nms=" << model_config.nms_threshold
		<< " input=" << model_config.input_width << "x" << model_config.input_height
		<< " intra_op_threads=" << model_config.intra_op_threads
		<< " cuda=" << (model_config.use_cuda ? "on" : "off")
		<< " device=" << model_config.cuda_device_id
		<< " profile_interval=" << model_config.profile_interval << "\n";

	out << "[tracking] detection_interval=" << tracking_config.detection_interval
		<< " target_class_id=" << tracking_config.target_class_id
		<< " iou_threshold=" << tracking_config.iou_threshold
		<< " reinit_iou_threshold=" << tracking_config.reinit_iou_threshold
		<< " max_lost_frames=" << tracking_config.max_lost_frames << "\n";

	out << "[kalman]   process_noise=" << tracking_config.kalman.process_noise
		<< " measurement_noise=" << tracking_config.kalman.measurement_noise
		<< " error_cov_post=" << tracking_config.kalman.error_cov_post << "\n";

	out << "[logging]  path=" << logging_config.path
		<< " level=" << logging_config.level
		<< " console=" << (logging_config.console ? "on" : "off") << "\n";

	out << "[display]  window=\"" << display_config.window_name
		<< "\" show_window=" << (display_config.show_window ? "on" : "off")
		<< " draw_fps=" << (display_config.draw_fps ? "on" : "off")
		<< " profile_interval=" << display_config.profile_interval << "\n";

	out << "----------------------------";

	return out.str();
}
