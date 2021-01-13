#pragma once
#include "util.h"
#include "MvCameraControl.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "potential_light.h"

class PotentialArmor {
public:
    PotentialLight light1;
    PotentialLight light2;

    int testint;

    PotentialArmor();
    PotentialArmor(PotentialLight one, PotentialLight two);

    bool matchAngle();
    bool matchArea();
    bool matchHeight();
    bool matchY();
    cv::Point2f getCenter();
    std::vector<cv::Point2f> getCorners();
    ArmorState validate();

private:
};
