#include<iostream>
#include<string>
#include<algorithm>
#include<opencv2/opencv.hpp>
#include<chrono>

#include"Camera.h"
#include"FPS.h"
#include"Logger.h"
#include"Config.h"
#include"Tracker.h"
#include"VideoRecorder.h"
#include"YOLODetector.h"
#include"MultiTracker.h"
/*
bool selectTarget(
    const std::vector<Detection>& detections,
    cv::Rect& target_box
);*/

int main()
{
    try
    {

        Config config("config/config.yaml");
        Logger logger("logs/tracking.log");
        logger.info("Application started");

        Camera camera(
            config.getCameraId(),
            config.getCameraWidth(),
            config.getCameraHeight()
        );

        logger.info("Camera initialized");

        YOLODetector detector(
            "models/yolo11n.onnx",
            0.5f,
            0.45f
        );
        logger.info("YOLO detector initialized");

        MultiTracker multiTracker;
        logger.info("MultiTracker initialized");

        FPS fps;
        VideoRecorder recorder(
            config.getVideoPath(),
            config.getVideoFps(),
            cv::Size(
                config.getCameraWidth(),
                config.getCameraHeight()
            )
        );

        logger.info("Video recorder initialized");

        int frame_count = 0;
        const int detection_interval = 15;

        /*
        bool tracking_init = false;
        bool tracking_success = false;
        cv::Rect tracking_box;
        int last_detection_frame = -100;
        const int detection_cooldown = 15;
        */

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
            /*
            bool need_detection = false;
            
            if (!tracking_init) {
                // 情况1：当前没有目标。只有在冷却时间过了之后，才允许重试
                if (frame_count - last_detection_frame >= detection_cooldown) {
                    need_detection = true;
                }
            }
            else if (frame_count % detection_interval == 0) {
                // 情况2：正在跟踪，到了固定间隔，去校准一下
                need_detection = true;
            }*/

            if (need_detection)
            {

                //last_detection_frame = frame_count;
                //logger.info("Running YOLO detection");

                std::vector<Detection> detections = detector.detect(frame);

                multiTracker.update(detections, frame);
                //cv::Rect target_box;
                /*
                if (selectTarget(detections, target_box))
                {
                    tracking_box = target_box;
                    tracker.init(frame, target_box);

                    tracking_init = true;
                    tracking_success = true;
                    //logger.info("Target found, CSRT initialized");
                }
                else
                {
                    tracking_init = false;
                    tracking_success = false;
                    //logger.warning("No target found in YOLO detection");
                }*/
            }
            else
            {
                multiTracker.update({}, frame);
                /*
                tracking_success = tracker.update(frame, tracking_box);

                if (!tracking_success)
                {
                    tracking_init = false;
                    //logger.warning("CSRT tracking failed,retrying YOLO...");
                }*/
            }
            auto t2 = std::chrono::high_resolution_clock::now();
            /*
            if (tracking_init && tracking_success)
            {
                cv::rectangle(
                    frame,
                    tracking_box,
                    cv::Scalar(0, 255, 0),
                    2
                );

                int label_y = std::max(0, tracking_box.y - 10);

                if (tracking_box.y < 30)
                {
                    label_y = tracking_box.y + 25;
                }
                else
                {
                    label_y = tracking_box.y - 10;
                }

                cv::putText(
                    frame,
                    "Tracking",
                    cv::Point(std::max(0, tracking_box.x), label_y),
                    cv::FONT_HERSHEY_SIMPLEX,
                    0.7,
                    cv::Scalar(0, 255, 0),
                    2
                );
            }
            else
            {
                cv::putText(
                    frame,
                    "searching...",
                    cv::Point(30, 40),
                    cv::FONT_HERSHEY_SIMPLEX,
                    1.0,
                    cv::Scalar(0, 0, 255),
                    2
                );
            }*/
            multiTracker.draw(frame);

            double curr_fps = fps.update();

            cv::putText(
                frame,
                "FPS: " + std::to_string(static_cast<int>(curr_fps)),
                cv::Point(frame.cols - 150, frame.rows - 20),
                cv::FONT_HERSHEY_SIMPLEX,
                0.8,
                cv::Scalar(0, 255, 0),
                2
            );

            recorder.write(frame);

            cv::imshow("YOLO + CSRT Hybrid Tracking", frame);
            auto t3 = std::chrono::high_resolution_clock::now();


            static int profile_count = 0;
            if (profile_count++ % 10 == 0) {
                std::cout << "读取摄像头: " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count()
                    << "ms | 检测跟踪: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
                    << "ms | 绘制+录像: " << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count() << "ms" << std::endl;
            }

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
/*
bool selectTarget(
    const std::vector<Detection>& detections,
    cv::Rect& target_box
)
{
    bool found = false;
    float best_confidence = 0.0f;

    for (const auto& detection : detections)
    {
        if (detection.class_id != 0) continue;

        if (!found || detection.confidence > best_confidence)
        {
            best_confidence = detection.confidence;
            target_box = detection.box;
            found = true;
        }
    }

    return found;
}*/