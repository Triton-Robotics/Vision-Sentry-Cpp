#pragma once
#include "MvCameraControl.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

class Detector {
public:
    //static Detector& get(cv::Mat* ptr) {
    //    static Detector instance(ptr);
    //    return instance;
    //};
    //static void Detect();

    // new code
    Detector();
    ~Detector() = default;

    // new code
    void DetectLive(cv::Mat &input);

private:
    static cv::Mat* addr;
    Detector(cv::Mat* ptr) {
        addr = ptr;
    };
    //~Detector() = default;
    Detector(const Detector&) = delete;
    Detector& operator=(const Detector&) = delete;
};
