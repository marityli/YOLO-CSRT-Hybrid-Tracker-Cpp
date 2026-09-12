#include<iostream>
#include<opencv2/opencv.hpp>

#include"Camera.h"
#include"FPS.h"
#include"VideoRecorder.h"
#include"Logger.h"
#include"Config.h"
#include"Tracker.h"



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

        FPS fps;

        cv::Mat frame = camera.read();

        if (frame.empty())
        {
            logger.error("Failed to read initial frame");
            return -1;
        }

        cv::Rect bbox = cv::selectROI(
            "Select Target",
            frame,
            false,
            false
        );

        if (bbox.width <= 0 || bbox.height <= 0)
        {
            logger.warning("No vaild target selected");
            camera.release();
            cv::destroyAllWindows();
            return 0;
        }

        Tracker tracker;
        tracker.init(frame, bbox);

        logger.info("Tracker initialized");

        VideoRecorder recorder(
            config.getVideoPath(),
            config.getVideoFps(),
            cv::Size(
                config.getCameraWidth(),
                config.getCameraHeight()
            )
        );

        logger.info("Video recorder initialized");

        while (true) {
            frame = camera.read();

            if (frame.empty())
            {
                logger.error("Failed to read frame");
                break;
            }

            bool success = tracker.update(frame, bbox);
            if (success)
            {
                cv::rectangle(
                    frame,
                    bbox,
                    cv::Scalar(0, 255, 0),
                    2
                );
                cv::putText(
                    frame,
                    "Tracking",
                    cv::Point(20, 40),
                    cv::FONT_HERSHEY_SIMPLEX,
                    1.0,
                    cv::Scalar(0, 255, 0),
                    2
                );
            }
            else
            {
                cv::putText(
                    frame,
                    "Target Lost",
                    cv::Point(20, 40),
                    cv::FONT_HERSHEY_SIMPLEX,
                    1.0,
                    cv::Scalar(0, 0, 255),
                    2
                );
            }

            double curr_fps = fps.update();

            cv::putText(
                frame,
                "FPS: " + std::to_string(curr_fps),
                cv::Point(20, 80), cv::FONT_HERSHEY_SIMPLEX,
                0.8,
                cv::Scalar(0, 255, 0),
                2
            );

            recorder.write(frame);

            cv::imshow("Object Tracking", frame);
            char key = static_cast<char>(cv::waitKey(1));

            if (key == 'q' || key == 27)
            {
                logger.info("User requested exit");
                break;
            }
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
