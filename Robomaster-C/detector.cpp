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

// new code

Detector::Detector() {

}

// changed from Detect 
void Detector::DetectLive(Mat &input) {
    // opencv declaration
    // removed input from declaration list
    Mat raw, hsv, mask, raw_debug, mask_debug;
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;

   /* input = *addr;*/
    resize(input, raw, Size(640, 512));
    cvtColor(raw, hsv, CV_RGB2HSV);

    inRange(hsv, Scalar(105, 200, 50), Scalar(135, 255, 255), mask);
    cvtColor(hsv, mask, COLOR_HSV2RGB);
    cvtColor(mask, mask, COLOR_RGB2GRAY);
    //threshold(mask, mask, 0, 255, THRESH_BINARY);
    mask.convertTo(mask, CV_8UC1, 255.0);
    //threshold(mask, mask, 0, 255, THRESH_BINARY);
    dilate(mask, mask, Mat(), Point(-1, -1), 2, 1, 1);
    findContours(mask, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

    // try draw contours 
    drawContours(raw, contours, -1, Scalar(0, 255, 255), 8, FILLED);
    //printf("contour size, %d\n", contours.size());

    //input = raw; // new code - not sure if this will work

#if DEBUG
    raw_debug = raw.clone();
    mask_debug = mask.clone();
    cvtColor(mask_debug, mask_debug, CV_GRAY2RGB);
#endif

    vector<PotentialLight> lights;
    Mat light_check = Mat(512, 640, CV_8UC3); // test for loop results

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

    imshow("good lights", light_check);

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
            //vector<Point2f> corners = armor.getCorners();
            //line(mask_debug, corners[0], corners[1], color, 1, 8);
            //line(mask_debug, corners[1], corners[3], color, 1, 8);
            //line(mask_debug, corners[3], corners[2], color, 1, 8);
            //line(mask_debug, corners[2], corners[0], color, 1, 8);

#endif // DEBUG

            //vector<Point2f> corners = armor.getCorners();

            //Scalar color = Scalar(255, 255, 0);
            //cv::line(raw, corners[0], corners[1], color, 1, 8);
            //cv::line(raw, corners[1], corners[3], color, 1, 8);
            //cv::line(raw, corners[3], corners[2], color, 1, 8);
            //cv::line(raw, corners[2], corners[0], color, 1, 8);
        }
    }

    // add code here to filter out to only get 1 pair that we want
    // if you have an odd number of valid lightbars, for ex if you have 3, then you know there's only max 1 armor plate
    // compute minimal distance and store armor with minimal distance into final_armor
    double min_distance = DBL_MAX;
    PotentialArmor final_armor;

    for (auto valid_armor : armors) {
        //vector<Point2f> corners = valid_armor.getCorners();
        //Scalar color = Scalar(255, 255, 0);
        //cv::line(raw, corners[0], corners[1], color, 1, 8);
        //cv::line(raw, corners[1], corners[3], color, 1, 8);
        //cv::line(raw, corners[3], corners[2], color, 1, 8);
        //cv::line(raw, corners[2], corners[0], color, 1, 8);

 /*       putText(raw, to_string(valid_armor.getDistance()[0]), Point2f(corners[0].x, corners[0].y + 10), CV_FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 0));
        putText(raw, to_string(valid_armor.getDistance()[1]), Point2f(corners[3].x, corners[3].y + 10), CV_FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0));*/


        if (min_distance > valid_armor.getDistance()[0]) {
            min_distance = valid_armor.getDistance()[0];
            final_armor = valid_armor;
        }
    }

    // if final_armor not null, then draw result
    if (min_distance != DBL_MAX) {
        vector<Point2f> corners = final_armor.getCorners();
        Scalar color = Scalar(255, 255, 0);
        cv::line(raw, corners[0], corners[1], color, 1, 8);
        cv::line(raw, corners[1], corners[3], color, 1, 8);
        cv::line(raw, corners[3], corners[2], color, 1, 8);
        cv::line(raw, corners[2], corners[0], color, 1, 8);
        circle(raw, final_armor.getCenter(), 15, Scalar(0, 255, 0), -1);
    }

    input = raw;

    //input = mask_debug;

//#if DEBUG
//
//    imshow("red", mask_debug);
//    imshow("raw image", raw_debug);
//
//    waitKey(0);
//
//#endif // DEBUG
}