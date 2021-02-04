#include "potential_light.h"
#include <stdio.h>
#include <conio.h>
#include <thread>
#include <string>

// TODO: CHECK MATH FOR ALL FUNCTIONS

using namespace cv;
using namespace std;

PotentialLight::PotentialLight() {}

PotentialLight::PotentialLight(RotatedRect box) {
    light_box = box;
    //printf("new light:\n-angle: %f\n-area: %f\n", getAngle(), (getWidth() * getHeight()));

    Mat light_check = Mat(512, 640, CV_8UC3);

    Mat corners_matrix;
    boxPoints(light_box, corners_matrix);

    // get the corners from the RotatedRect
    vector<Point2f> unordered_corners(4);
    corners = unordered_corners;

    Point2f rect_points[4];
    light_box.points(rect_points);

    printf("Before loop\n");

    for (int i = 0; i < 4; i++)
        corners[i] = rect_points[i];

    // sort the corners based on y-value
    for (int i = 0; i < 4; i++) {
        Point2f current = corners[i];

        for (int j = i; j < 4; j++) {
            if (corners[j].y < current.y) {
                corners[i] = corners[j];
                corners[j] = current;
                current = corners[i];
            }
        }
    }

    //for (auto coord : corners) {
    //    printf("x is: %f\n", coord.x);
    //    printf("y is: %f\n", coord.y);
    //    circle(light_check, coord, 10, Scalar(0, 255, 255), -1);
    //}
    //printf("After loop\n\n");
    //imshow("light corners", light_check);
    circle(light_check, box.center, 30, Scalar(0, 255, 255), -1);
    string angle = to_string(int(box.angle) % 360);

    Point2f top = (corners[0] + corners[1]) / 2;
    Point2f bottom = (corners[2] + corners[3]) / 2;

    // need to go the other way, since origin point is at top left.
    float degrees = atan2(bottom.y - top.y, top.x - bottom.x) * 180 / CV_PI;
    string deg = to_string(degrees);

    putText(light_check, deg, Point2f(box.center.x + 50, box.center.y + 50), CV_FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 0));

    putText(light_check, angle, Point2f(box.center.x + 50, box.center.y), CV_FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255));
    imshow("angles", light_check);
};

vector<Point2f> PotentialLight::getCorners() {
    return corners;
}

float PotentialLight::getWidth() {
    float delta_x = abs(corners[0].x - corners[1].x);
    float delta_y = abs(corners[0].y - corners[1].y);

    return sqrt(pow(delta_x, 2) + pow(delta_y, 2));
}

float PotentialLight::getHeight() {
    float delta_x = abs(corners[0].x - corners[2].x);
    float delta_y = abs(corners[0].y - corners[2].y);

    return sqrt(pow(delta_x, 2) + pow(delta_y, 2));
}

// maybe we can just get the angle from rotatedRect in constructor
// maybe store the angle in an instance variable 
// 0 90 180 270 etc are valid upright angles
float PotentialLight::getAngle() {

    vector<Point2f> corners = getCorners();

    Point2f top = (corners[0] + corners[1]) / 2;
    Point2f bottom = (corners[2] + corners[3]) / 2;

    // need to go the other way, since origin point is at top left.
    float degrees = atan2(bottom.y - top.y, top.x - bottom.x) * 180 / CV_PI;

    return degrees;
}

Point2f PotentialLight::getTop() {
    return (corners[0] + corners[1]) / 2;
}

Point2f PotentialLight::getBottom() {
    return (corners[2] + corners[3]) / 2;
}

Point2f PotentialLight::getCenter() {
    return (corners[0] + corners[3]) / 2;

    //return light_box.center;
}

LightState PotentialLight::validate() {
    float angle = getAngle();
    // angle should be within valid range
    if (!(angle < 120 && angle > 60)) {
        // printf("Angle wrong: %f\n", angle);
        return LightState::ANGLE_ERROR;
    }
    float width = getWidth();
    float height = getHeight();

    // ratio of width : height should be acceptable
    // TODO: tune 2nd condition
    if (width > height || height > width * 10) {
        //printf("ratio wrong");
        return LightState::RATIO_ERROR;
    }

    float area = width * height;
    // light should not be too big or small (100 might be a good value)
    if (area < 100) {
        //printf("wrong area: %f\n", area); 
        return LightState::AREA_ERROR;
    }

    // passed all tests, return true
    return LightState::NO_ERROR;
}