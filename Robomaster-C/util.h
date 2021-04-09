#pragma once
#include <functional>
#include <opencv2/core/core.hpp>

// changed to 0
#define DEBUG 0
#define COLOR 1 // 1 for blue, 0 for red

// some enums, don't touch
const enum class LightState { NONE, ALL, NO_ERROR, ANGLE_ERROR, RATIO_ERROR, AREA_ERROR };
const enum class ArmorState { NONE, ALL, NO_ERROR, ANGLE_ERROR, HEIGHT_ERROR, Y_ERROR, PROP_ERROR };

// debug parameter
const LightState LIGHT_VIS_TYPE = LightState::NO_ERROR;
const ArmorState ARMOR_VIS_TYPE = ArmorState::NO_ERROR;

// util functions
void runFPS(std::function<void()> f, int n);
