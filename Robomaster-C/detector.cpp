#include "detector.h"
#include "potential_light.h"
#include "potential_armor.h"
#include "util.h"
#include <stdio.h>
#include <conio.h>
#include <thread>
#include <iostream>

using namespace cv;
using namespace std;

cv::Mat* Detector::addr = NULL;

void Detector::Detect() {
    // opencv declaration
    Mat input, raw, hsv, mask, raw_debug, mask_debug;
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;

    input = *addr;
    resize(input, raw, Size(640, 512));
    cvtColor(raw, hsv, CV_RGB2HSV);

    inRange(hsv, Scalar(105, 200, 50), Scalar(135, 255, 255), mask);
    dilate(mask, mask, Mat(), Point(-1, -1), 2, 1, 1);

    findContours(mask, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

#if DEBUG
    raw_debug = raw.clone();
    mask_debug = mask.clone();
    cvtColor(mask_debug, mask_debug, CV_GRAY2RGB);
#endif

    vector<PotentialLight> lights;

    for (int i = 0; i < contours.size(); i++) {
        PotentialLight light = PotentialLight(minAreaRect(contours[i]));
        LightState light_state = light.validate();

        if (light_state == LightState::NO_ERROR) {
            lights.push_back(light);
        }

#if DEBUG
        if (LIGHT_VIS_TYPE == LightState::NONE ||
            (LIGHT_VIS_TYPE != LightState::ALL && LIGHT_VIS_TYPE != light_state)) {
            continue;
        }

        for (int j = 0; j < 4; j++) {
            int c = 255 - (50 * j);
            Scalar color = Scalar(c, c, c);
            circle(mask_debug, light.getCorners()[j], 3, color, CV_FILLED, 8, 0);
        }

        // draw poles
        Scalar north = Scalar(0, 0, 255);
        circle(mask_debug, light.getTop(), 1, north, CV_FILLED, 8, 0);
        Scalar south = Scalar(0, 211, 255);
        circle(mask_debug, light.getBottom(), 1, south, CV_FILLED, 8, 0);

        // Point2f plateCenter = lights[i].getCenter();
        Scalar color;
        switch (light_state) {
        case LightState::NO_ERROR:
            color = Scalar(0, 255, 0);
            break;
        case LightState::ANGLE_ERROR:
            color = Scalar(255, 0, 0);
            break;
        case LightState::RATIO_ERROR:
            color = Scalar(0, 0, 255);
            break;
        case LightState::AREA_ERROR:
            color = Scalar(255, 255, 0);
            break;
        }

        /* circle(mask_debug, plateCenter, 4, color, CV_FILLED, 8, 0); */

        vector<Point2f> corners = light.getCorners();
        line(mask_debug, corners[0], corners[1], color, 1, 8);
        line(mask_debug, corners[1], corners[3], color, 1, 8);
        line(mask_debug, corners[3], corners[2], color, 1, 8);
        line(mask_debug, corners[2], corners[0], color, 1, 8);

#endif // DEBUG

    }

    vector<PotentialArmor> armors;
    // Get the center point of the plate
    for (int i = 0; i < lights.size(); i++) {
        for (int j = i + 1; j < lights.size(); j++) {

            PotentialArmor armor = PotentialArmor(lights[i], lights[j]);
            ArmorState armor_state = armor.validate();

            if (armor_state == ArmorState::NO_ERROR) {
                armors.push_back(armor);
            }

#if DEBUG

            if (ARMOR_VIS_TYPE == ArmorState::NONE ||
                (ARMOR_VIS_TYPE != ArmorState::ALL && ARMOR_VIS_TYPE != armor_state)) {
                continue;
            }

            Scalar color;
            switch (armor_state) {
            case ArmorState::NO_ERROR:
                color = Scalar(0, 255, 0);
                break;
            case ArmorState::ANGLE_ERROR:
                color = Scalar(255, 0, 0);
                break;
            case ArmorState::HEIGHT_ERROR:
                color = Scalar(0, 0, 255);
                break;
            case ArmorState::Y_ERROR:
                color = Scalar(0, 255, 255);
                break;
            }
            /*
            int size = (light_state == 1) ? 10 : 5;
            circle(mask_debug, armor.getCenter(), size, color, CV_FILLED, 8, 0);
            */
            vector<Point2f> corners = armor.getCorners();
            line(mask_debug, corners[0], corners[1], color, 1, 8);
            line(mask_debug, corners[1], corners[3], color, 1, 8);
            line(mask_debug, corners[3], corners[2], color, 1, 8);
            line(mask_debug, corners[2], corners[0], color, 1, 8);

#endif // DEBUG
        }
    }

#if DEBUG

    imshow("red", mask_debug);
    imshow("raw image", raw_debug);

    waitKey(0);

#endif // DEBUG
}