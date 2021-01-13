
#pragma once
#include "util.h"
#include "MvCameraControl.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

class PotentialLight {
public:
    cv::RotatedRect light_box;
    std::vector<cv::Point2f> corners;

    PotentialLight();
    PotentialLight(cv::RotatedRect box);
    std::vector<cv::Point2f> getCorners();
    float getWidth();
    float getHeight();
    float getAngle();
    cv::Point2f getTop();
    cv::Point2f getBottom();
    cv::Point2f getCenter();
    LightState validate();

private:
};
