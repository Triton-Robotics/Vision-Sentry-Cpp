#include "potential_armor.h"
#include "potential_light.h"
#include <stdio.h>
#include <conio.h>
#include <thread>

using namespace cv;
using namespace std;

PotentialArmor::PotentialArmor() {}

PotentialArmor::PotentialArmor(PotentialLight one, PotentialLight two) {
    light1 = one;
    light2 = two;
};

bool PotentialArmor::matchAngle() {
    float difference = light1.getAngle() - light2.getAngle();
    //printf("%f\n", difference);
    return abs(difference) < 20;
}

bool PotentialArmor::matchArea() {
    float area1 = light1.getHeight() * light1.getWidth();
    float area2 = light2.getHeight() * light2.getWidth();

    float prop_areas = area1 / area2;

    return (prop_areas < 2 && prop_areas > 0.5);
}

bool PotentialArmor::matchHeight() {
    float height1 = light1.getHeight();
    float height2 = light2.getHeight();

    float prop_heights = height1 / height2;

    //printf("%f\n", propWidths);

    return (prop_heights < 1.5 && prop_heights > 0.5);
}

bool PotentialArmor::matchY() {
    // added 10 for buffer
    float top1 = light1.getTop().y - 10;
    float bottom1 = light1.getBottom().y + 10;

    //printf("Potential armor y-check:\n");

    //printf("one: %f, %f\n", top1, bottom1);

    float top2 = light2.getTop().y - 10;
    float bottom2 = light2.getBottom().y + 10;

    //printf("two: %f, %f\n\n", top2, bottom2);

    // is the top of two in the y-bounds of one?
    if (top2 <= bottom1 && top2 >= top1) {
        return true;
    }
    // is the bottom of two in the y-bounds of one?
    else if (bottom2 <= bottom1 && bottom2 >= top1) {
        return true;
    }
    // is the top of one in the y-bounds of two?
    else if (top1 <= bottom2 && top1 >= top2) {
        return true;
    }
    // is the top of one in the y-bounds of two?
    else if (bottom1 <= bottom2 && bottom1 >= top2) {
        return true;
    }

    //printf("Failed!\n\n");

    return false;
}

cv::Point2f PotentialArmor::getCenter() {
    return (light1.getCenter() + light2.getCenter()) / 2;
}

vector<Point2f> PotentialArmor::getCorners() {
    vector<Point2f> corners{ light1.getTop(), light2.getTop(),
        light1.getBottom(), light2.getBottom() };
    return corners;
}

ArmorState PotentialArmor::validate() {
    if (!matchAngle()) {
        return ArmorState::ANGLE_ERROR;
    }
    else if (!matchHeight()) {
        return ArmorState::HEIGHT_ERROR;
    }
    else if (!matchY()) {
        return ArmorState::Y_ERROR;
    }
    else {
        return ArmorState::NO_ERROR;
    }
}