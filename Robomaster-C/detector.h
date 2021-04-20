#pragma once
#include "MvCameraControl.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

class Detector {
public:

    Detector();
    ~Detector() = default;

    void DetectLive(cv::Mat &input);

private:
    static cv::Mat* addr;
    cv::Point2f prev_center;
    double prev_distance;
    double prev_area;
    Detector(cv::Mat* ptr) {
        addr = ptr;
    };

    Detector(const Detector&) = delete;
    Detector& operator=(const Detector&) = delete;
};
