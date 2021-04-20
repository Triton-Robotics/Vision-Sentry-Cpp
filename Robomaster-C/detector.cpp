#include "detector.h"
#include "potential_light.h"
#include "potential_armor.h"
#include "util.h"
#include <stdio.h>
#include <conio.h>
#include <thread>
#include <limits>
#include <iostream>

using namespace cv;
using namespace std;

cv::Mat* Detector::addr = NULL;

Detector::Detector() {
    prev_center = Point2f(320, 256);
    prev_distance = 410; // diagonal frame distance from the center
}

void Detector::DetectLive(Mat &input) {
    Mat raw, hsv, blue_mask, red_mask, mask, raw_debug, mask_debug, color_mask, mask_results;
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;

    // color ranges for checking team colors
    Scalar low_blue(140, 0, 0); 
    Scalar high_blue(255, 255, 255); 

    Scalar low_red(0, 0, 200);
    Scalar high_red(255, 255, 255);

    resize(input, raw, Size(640, 512)); // center is (320, 256)
    cvtColor(raw, hsv, CV_RGB2HSV);

    // choose which mask to use based on the team color - change the color ENUM in util.h for now
    if (COLOR) {
        inRange(raw, low_blue, high_blue, color_mask);
    }
    else {
        inRange(raw, low_red, high_red, color_mask);
    }

    // make regions thicker and make all shades of white above 127 255 for noise removal
    threshold(color_mask, color_mask, 127, 255, THRESH_BINARY);
    dilate(color_mask, color_mask, Mat(), Point(-1, -1), 2, 1, 1);

    // applying filtered regions from mask to raw image data
    bitwise_and(raw, raw, mask=color_mask);

    findContours(color_mask, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

    // try draw contours 
    //drawContours(raw, contours, -1, Scalar(0, 255, 255), 8, FILLED);

#if DEBUG
    raw_debug = raw.clone();
    mask_debug = mask.clone();
    cvtColor(mask_debug, mask_debug, CV_GRAY2RGB);
#endif

    vector<PotentialLight> lights;
    Mat light_check = Mat(512, 640, CV_8UC3); // test for loop results

    // go through all contours and run validation checks
    for (int i = 0; i < contours.size(); i++) {
        PotentialLight light = PotentialLight(minAreaRect(contours[i]));
        LightState light_state = light.validate();

        if (light_state == LightState::NO_ERROR) {
            lights.push_back(light);
            circle(light_check, light.corners[0], 50, Scalar(0, 255, 0), -1);
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
        cv::line(mask_debug, corners[0], corners[1], color, 1, 8);
        cv::line(mask_debug, corners[1], corners[3], color, 1, 8);
        cv::line(mask_debug, corners[3], corners[2], color, 1, 8);
        cv::line(mask_debug, corners[2], corners[0], color, 1, 8);

#endif // DEBUG

    }

    vector<PotentialArmor> armors;
    // Get the center point of the plate
    // go through every single possible pairs of light bar and run validation checks on each pair
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
            //vector<Point2f> corners = armor.getCorners();
            //line(mask_debug, corners[0], corners[1], color, 1, 8);
            //line(mask_debug, corners[1], corners[3], color, 1, 8);
            //line(mask_debug, corners[3], corners[2], color, 1, 8);
            //line(mask_debug, corners[2], corners[0], color, 1, 8);

#endif // DEBUG
        }
    }
    
    // set up variables for choosing armor closest to center
    double min_distance = DBL_MAX;
    double closest_to_center = DBL_MAX;
    double closest_to_prev = DBL_MAX;
    double distance_to_center = 0;
    double distance_to_prev = 0;
    double radius_scaler = 2;
    PotentialArmor final_armor;

    for (auto valid_armor : armors) {

        // compute the distance to the center from center of current valid armor and center of camera
        distance_to_center = sqrt(pow((320 - valid_armor.getCenter().x), 2) + pow((256 - valid_armor.getCenter().y), 2));
        distance_to_prev = sqrt(pow((prev_center.x - valid_armor.getCenter().x), 2) + pow((prev_center.y - valid_armor.getCenter().y), 2));

        if (distance_to_prev < closest_to_prev) {
            closest_to_prev = distance_to_prev;
            final_armor = valid_armor;
        }


        //if (min_distance > valid_armor.getDistance()[0]) {
        //    min_distance = valid_armor.getDistance()[0];
        //    final_armor = valid_armor;
        //}
    }

    // 

    // if final_armor not null, then draw result
    if (closest_to_prev <= radius_scaler * prev_distance) {
        prev_center = final_armor.getCenter();
        prev_distance = final_armor.getDistance[0];
        // TODO return coordinate of detected result
    } else {
        // reset to defaults
        prev_center = Point2f(320, 256);
        prev_distance = 410;
    }

    input = raw;

#if DEBUG

    imshow("red", mask_debug);
    imshow("raw image", raw_debug);

    waitKey(0);

#endif // DEBUG
}