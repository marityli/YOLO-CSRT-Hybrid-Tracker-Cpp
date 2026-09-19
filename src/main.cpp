#include<iostream>
#include<string>
#include<algorithm>
#include<opencv2/opencv.hpp>
#include<chrono>

#include"Camera.h"
#include"FPS.h"
#include"Logger.h"
#include"Config.h"
#include"VideoRecorder.h"
#include"YOLODetector.h"
#include"MultiTracker.h"


int main(int argc, char** argv)
{
    // 允许通过命令行参数指定配置文件，默认 config/config.yaml
    const std::string config_path =
        (argc > 1) ? argv[1] : "config/config.yaml";

    try
    {

        Config config(config_path);

        Logger logger(
            config.logging().path,
            config.logging().level,
            config.logging().console
        );
        logger.info("Application started");
        logger.info("Config loaded from: " + config_path);
        logger.info(config.describe());

        const CameraConfig& camera_config = config.camera();
        const ModelConfig& model_config = config.model();
        const VideoConfig& video_config = config.video();
        const TrackingConfig& tracking_config = config.tracking();
        const DisplayConfig& display_config = config.display();

        Camera camera(
            camera_config.id,
            camera_config.width,
            camera_config.height
        );

        logger.info("Camera initialized");

        YOLODetector detector(
            model_config.path,
            model_config.confidence_threshold,
            model_config.nms_threshold,
            model_config.input_width,
            model_config.input_height,
            model_config.intra_op_threads,
            model_config.use_cuda,
            model_config.cuda_device_id,
            model_config.profile_interval
        );
        logger.info("YOLO detector initialized");

        MultiTracker multiTracker(tracking_config);
        logger.info("MultiTracker initialized");

        FPS fps;
        VideoRecorder recorder(
            video_config.path,
            video_config.fps,
            cv::Size(
                camera_config.width,
                camera_config.height
            ),
            video_config.fourcc
        );

        if (!recorder.isOpened())
        {
            logger.warning(
                "Video recorder is not available, recording is disabled"
            );
        }

        logger.info("Video recorder initialized");

        int frame_count = 0;
        const int detection_interval = tracking_config.detection_interval;

        while (true) {
            auto t0 = std::chrono::high_resolution_clock::now();
            cv::Mat frame = camera.read();
            auto t1 = std::chrono::high_resolution_clock::now();

            if (frame.empty())
            {
                logger.error("Failed to read frame");
                break;
            }

            bool need_detection = (frame_count % detection_interval == 0);

            if (need_detection)
            {

                std::vector<Detection> detections = detector.detect(frame);

                multiTracker.update(detections, frame);

            }
            else
            {
                multiTracker.update({}, frame);
            }
            auto t2 = std::chrono::high_resolution_clock::now();

            multiTracker.draw(frame);

            double curr_fps = fps.update();

            if (display_config.draw_fps)
            {
                cv::putText(
                    frame,
                    "FPS: " + std::to_string(static_cast<int>(curr_fps)),
                    cv::Point(frame.cols - 150, frame.rows - 20),
                    cv::FONT_HERSHEY_SIMPLEX,
                    0.8,
                    cv::Scalar(0, 255, 0),
                    2
                );
            }

            recorder.write(frame);

            if (display_config.show_window)
            {
                cv::imshow(display_config.window_name, frame);
            }
            auto t3 = std::chrono::high_resolution_clock::now();


            if (display_config.profile_interval > 0
                && frame_count % display_config.profile_interval == 0) {
                std::cout << "读取摄像头: " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count()
                    << "ms | 检测跟踪: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
                    << "ms | 绘制+录像: " << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count() << "ms" << std::endl;
            }

            // show_window=false 时依然保留 waitKey，作为消息泵并保留 q 退出
            char key = static_cast<char>(cv::waitKey(1));

            if (key == 'q' || key == 27)
            {
                logger.info("User requested exit");
                break;
            }

            frame_count++;

        }

        recorder.release();
        camera.release();
        cv::destroyAllWindows();
        logger.info("Application stopped");
    }
    catch (const std::exception& e)
    {
        std::cerr << "Application error: "
            << e.what()
            << std::endl;

        return -1;
    }


    return 0;
}
