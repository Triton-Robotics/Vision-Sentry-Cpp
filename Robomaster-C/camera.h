#pragma once
#include "util.h"
#include "MvCameraControl.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <Windows.h>


class Camera {
public:

    //typedef HWND(WINAPI* PROCGETCONSOLEWINDOW)();
    //PROCGETCONSOLEWINDOW GetConsoleWindowAPI;

    cv::Mat* image_ptr;
    int input;
    void* handle;

    Camera();
    Camera(std::string filename);
    cv::Mat* GetAddress();
    void DummyWorkThread();
    void WorkThread(void* pUser);


private:
    cv::Mat image;
    unsigned int payload_size;
    unsigned int raw_data_size;
    unsigned int rgb_data_size;
    unsigned char* rgb_data_ptr;
    unsigned char* raw_data_ptr;
    MV_FRAME_OUT_INFO_EX image_info;
    int ret;

    enum Input {
        FROM_CAMERA = 0,
        FROM_FILE = 1
    };

    void PrintDeviceInfo(MV_CC_DEVICE_INFO* pstMVDevInfo);
    cv::Point2f GetCenterOfPlate(cv::RotatedRect rect1, cv::RotatedRect rect2);
};
