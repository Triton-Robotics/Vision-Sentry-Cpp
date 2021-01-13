#include "camera.h"
#include <stdio.h>
#include <conio.h>
#include <thread>
#include <exception>
#include "opencv2/core.hpp"
#include "exception.h"


using namespace cv;
using namespace std;


Camera::Camera() {
    input = FROM_CAMERA;
    ret = MV_OK;
    handle = NULL;

    // Enum device
    MV_CC_DEVICE_INFO_LIST stDeviceList;
    memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
    ret = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
    if (MV_OK != ret) {
        printf("Enum Devices fail! ret [0x%x]\n", ret);
    }

    if (stDeviceList.nDeviceNum > 0) {
        for (unsigned int i = 0; i < stDeviceList.nDeviceNum; i++) {
            printf("[device %d]:\n", i);
            MV_CC_DEVICE_INFO* pDeviceInfo = stDeviceList.pDeviceInfo[i];
            if (NULL == pDeviceInfo) {
                printf("null device");
            }
            PrintDeviceInfo(pDeviceInfo);
        }
    }
    else {
        // printf("Find No Devices!\n");
        throw NoDeviceException();
    }

    unsigned int nIndex = 0;

    if (nIndex >= stDeviceList.nDeviceNum) {
        // printf("Input error!\n");
        throw InputException();
    }

    // Select device and create handle
    ret = MV_CC_CreateHandle(&handle, stDeviceList.pDeviceInfo[nIndex]);
    if (MV_OK != ret) {
        printf("Create Handle fail! ret [0x%x]\n", ret);
    }

    // Open device
    ret = MV_CC_OpenDevice(handle);
    if (MV_OK != ret) {
        printf("Open Device fail! ret [0x%x]\n", ret);
    }

    // Set trigger mode as off
    ret = MV_CC_SetEnumValue(handle, "TriggerMode", 0);
    if (MV_OK != ret) {
        printf("Set Trigger Mode fail! ret [0x%x]\n", ret);
    }

    // Set exposure time
    ret = MV_CC_SetExposureTime(handle, 500);
    if (MV_OK != ret) {
        printf("Set Exposure fail! ret [0x%x]\n", ret);
    }

    // Get payload size
    MVCC_INTVALUE stParam;
    memset(&stParam, 0, sizeof(MVCC_INTVALUE));
    ret = MV_CC_GetIntValue(handle, "PayloadSize", &stParam);
    if (MV_OK != ret) {
        printf("Get PayloadSize fail! ret [0x%x]\n", ret);
    }
    payload_size = stParam.nCurValue;

    // Start grab image
    ret = MV_CC_StartGrabbing(handle);
    if (MV_OK != ret) {
        printf("Start Grabbing fail! ret [0x%x]\n", ret);
    }

    raw_data_ptr = (unsigned char*)malloc(sizeof(unsigned char) * (payload_size));
    raw_data_size = payload_size;
    image_info = { 0 };
    memset(&image_info, 0, sizeof(MV_FRAME_OUT_INFO_EX));

    // test image receiving - first param was NULL - changed to handle
    ret = MV_CC_GetOneFrameTimeout(handle, raw_data_ptr, raw_data_size, &image_info, 1000);
    if (ret == MV_OK) {
        printf("GetFrame Test Passed.");
    }
    else {
        printf("GetFrame Test Failed.");
    }

    // initialize RGB memory
    rgb_data_ptr = (unsigned char*)malloc(image_info.nWidth * image_info.nHeight * 3);
    if (NULL == rgb_data_ptr) {
        printf("malloc rgb_data_ptr fail!");
    }
    rgb_data_size = image_info.nWidth * image_info.nHeight * 3;
};

Camera::Camera(string filename) {
    input = FROM_FILE;
    image = imread(filename);
    image_ptr = &image;
}


cv::Mat* Camera::GetAddress() {
    return image_ptr;
}


Point2f Camera::GetCenterOfPlate(RotatedRect rect1, RotatedRect rect2) {
    double centerX = (rect1.center.x + rect2.center.x) / 2.0;
    double centerY = (rect1.center.y + rect2.center.y) / 2.0;
    return Point2f(centerX, centerY);
}

void Camera::DummyWorkThread() {
    while (1) {
        this_thread::sleep_for(chrono::milliseconds(100000));
    }
}
void Camera::WorkThread(void* pUser) {

    // opencv declaration
    Mat raw, hsv, red1, red2, red, mask;
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;

    // main loop
    clock_t t0 = clock();
    while (1) {
        clock_t t1 = clock();
        ret = MV_CC_GetOneFrameTimeout(pUser, raw_data_ptr, raw_data_size, &image_info, 1000);

        if (ret == MV_OK) {
            printf("Get One Frame: Width[%d], Height[%d], nFrameNum[%d]\n",
                image_info.nWidth, image_info.nHeight, image_info.nFrameNum);

            // Convert pixel format 
            MV_CC_PIXEL_CONVERT_PARAM stConvertParam = { 0 };
            memset(&stConvertParam, 0, sizeof(MV_CC_PIXEL_CONVERT_PARAM));

            stConvertParam.nWidth = image_info.nWidth;                 //image width
            stConvertParam.nHeight = image_info.nHeight;               //image height
            stConvertParam.pSrcData = raw_data_ptr;                            //input data buffer
            stConvertParam.nSrcDataLen = image_info.nFrameLen;         //input data size
            stConvertParam.enSrcPixelType = image_info.enPixelType;    //input pixel format
            stConvertParam.enDstPixelType = PixelType_Gvsp_RGB8_Packed; //coutput pixel format
            stConvertParam.pDstBuffer = rgb_data_ptr;                    //output data buffer
            stConvertParam.nDstBufferSize = rgb_data_size;            //output buffer size
            ret = MV_CC_ConvertPixelType(pUser, &stConvertParam);
            if (MV_OK != ret) {
                printf("Convert Pixel Type fail! ret [0x%x]\n", ret);
                break;
            }

            // new code
            //cv::Mat srcImage;
            ///*if (image_info.enPixelType == PixelType_Gvsp_Mono8)
            //{*/
            //for (unsigned int j = 0; j < image_info.nHeight; j++)
            //        {
            //            for (unsigned int i = 0; i < image_info.nWidth; i++)
            //            {
            //                unsigned char red = raw_data_ptr[j * (image_info.nWidth * 3) + i * 3];
            //                raw_data_ptr[j * (image_info.nWidth * 3) + i * 3] = raw_data_ptr[j * (image_info.nWidth * 3) + i * 3 + 2];
            //                raw_data_ptr[j * (image_info.nWidth * 3) + i * 3 + 2] = red;
            //            }
            //        }
            //    srcImage = cv::Mat(image_info.nHeight, image_info.nWidth, CV_8UC3, raw_data_ptr);
                //imshow("Display window", srcImage);
                //int k = waitKey(0);
                
            //}
            //else if (image_info.enPixelType == PixelType_Gvsp_RGB8_Packed)
            //{
            //    for (unsigned int j = 0; j < image_info.nHeight; j++)
            //    {
            //        for (unsigned int i = 0; i < image_info.nWidth; i++)
            //        {
            //            unsigned char red = raw_data_ptr[j * (image_info.nWidth * 3) + i * 3];
            //            raw_data_ptr[j * (image_info.nWidth * 3) + i * 3] = raw_data_ptr[j * (image_info.nWidth * 3) + i * 3 + 2];
            //            raw_data_ptr[j * (image_info.nWidth * 3) + i * 3 + 2] = red;
            //        }
            //    }
            //    //RGB2BGR(raw_data_ptr, image_info.nWidth, image_info.nHeight);
            //    srcImage = cv::Mat(image_info.nHeight, image_info.nWidth, CV_8UC3, raw_data_ptr);
            //}
            //else
            //{
            //    printf("unsupported pixel format\n");
            //}

            // added code
            //printf("here\n");
            //HMODULE hKernel32 = GetModuleHandle((LPCWSTR)"kernel32.dll");
            //GetConsoleWindowAPI = (PROCGETCONSOLEWINDOW)GetProcAddress(hKernel32, (LPCSTR)"GetConsoleWindow");
            //HWND hWnd = GetConsoleWindowAPI(); //window handle

            //////Display images
            //ret = MV_CC_Display(handle, hWnd);
            //if (ret != MV_OK) {
            //    printf("display frame fail!\n");
            //}


        }
        else {
            printf("No data[0x%x]\n", ret);
        }
    }

    free(raw_data_ptr);
}


void Camera::PrintDeviceInfo(MV_CC_DEVICE_INFO* pstMVDevInfo) {
    if (NULL == pstMVDevInfo) {
        printf("The Pointer of pstMVDevInfo is NULL!\n");
    }
    if (pstMVDevInfo->nTLayerType == MV_GIGE_DEVICE) {
        int nIp1 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24);
        int nIp2 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16);
        int nIp3 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8);
        int nIp4 = (pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff);

        // print current ip and user defined name
        printf("CurrentIp: %d.%d.%d.%d\n", nIp1, nIp2, nIp3, nIp4);
        printf("UserDefinedName: %s\n\n", pstMVDevInfo->SpecialInfo.stGigEInfo.chUserDefinedName);
    }
    else if (pstMVDevInfo->nTLayerType == MV_USB_DEVICE) {
        printf("UserDefinedName: %s\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chUserDefinedName);
        printf("Serial Number: %s\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chSerialNumber);
        printf("Device Number: %d\n\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.nDeviceNumber);
    }
    else {
        printf("Not support.\n");
    }
}