#include "camera.h"
#include <stdio.h>
#include <iostream>
#include <conio.h>
#include <thread>
#include <exception>
#include <string>
#include <vld.h>
#include "opencv2/core.hpp"
#include "exception.h"
#include "detector.h"


using namespace cv;
using namespace std;

typedef HWND(WINAPI* PROCGETCONSOLEWINDOW)();
PROCGETCONSOLEWINDOW GetConsoleWindowAPI;

bool Convert2Mat(MV_CC_PIXEL_CONVERT_PARAM* pstImageInfo, unsigned char* pData, cv::Mat& srcImage);
int RGB2BGR(unsigned char* pRgbData, unsigned int nWidth, unsigned int nHeight);


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

    // list out all devices found
    //if (stDeviceList.nDeviceNum > 0) {
    //    for (unsigned int i = 0; i < stDeviceList.nDeviceNum; i++) {
    //        printf("[device %d]:\n", i);
    //        MV_CC_DEVICE_INFO* pDeviceInfo = stDeviceList.pDeviceInfo[i];
    //        if (NULL == pDeviceInfo) {
    //            printf("null device");
    //        }
    //        PrintDeviceInfo(pDeviceInfo);
    //    }
    //}
    //else {
    //    throw NoDeviceException();
    //}

    unsigned int nIndex = 0;

    // check if valid device found
    if (nIndex >= stDeviceList.nDeviceNum) {
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
    MVCC_INTVALUE stIntvalue = { 0 };
    ret = MV_CC_GetIntValue(handle, "PayloadSize", &stIntvalue);
    if (MV_OK != ret) {
        printf("Get PayloadSize fail! ret [0x%x]\n", ret);
    }
    // buffer size - need to be made more efficient sizewise - was * 2 1.49 was too small 1.5 works 
    nBufSize = stIntvalue.nCurValue * 1.50; //One frame data size + reserved bytes (handled in SDK) was 2048

    unsigned int nTestFrameSize = 0;
    pFrameBuf = (unsigned char*)malloc(nBufSize);

    memset(&stInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));

    // Start grab image
    ret = MV_CC_StartGrabbing(handle);
    if (MV_OK != ret) {
        printf("Start Grabbing fail! ret [0x%x]\n", ret);
    }
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

    Detector detector;

    // Test one frame display
    MV_FRAME_OUT stOutFrame;
    memset(&stOutFrame, 0, sizeof(MV_FRAME_OUT));
    MV_DISPLAY_FRAME_INFO stDisplayOneFrame;
    memset(&stDisplayOneFrame, 0, sizeof(MV_DISPLAY_FRAME_INFO));
    int nTestFrameSize = 0;
    int nRet = -1;

    MV_CC_PIXEL_CONVERT_PARAM stParam;
    memset(&stParam, 0, sizeof(MV_CC_PIXEL_CONVERT_PARAM));

    unsigned char* pImage = (unsigned char*)malloc(nBufSize);

    int frame_num = 0;
    while (1)
    {
        nRet = MV_CC_GetImageBuffer(handle, &stOutFrame, 1000);
        if (MV_OK != nRet)
        {
            printf("Jank did not work\n");
        }
        
        // setting up frame parameters
        stParam.pSrcData = stOutFrame.pBufAddr;              //Original image data - is pFrameBuf
        stParam.nSrcDataLen = stOutFrame.stFrameInfo.nFrameLen;         //Length of original image data
        stParam.enSrcPixelType = stOutFrame.stFrameInfo.enPixelType;       //Pixel format of original image
        stParam.nWidth = stOutFrame.stFrameInfo.nWidth;            //Image width
        stParam.nHeight = stOutFrame.stFrameInfo.nHeight;           //Image height

        //Target data
        stParam.enDstPixelType = PixelType_Gvsp_RGB8_Packed;     //Pixel format type needs to be saved, it will transform to MONO8 format
        stParam.nDstBufferSize = nBufSize;             //Size of storage node
        stParam.pDstBuffer = pImage;                   //Buffer for the output data,used to save the transformed data.

        // convert input frame pixel type to desired pixel type
        nRet = MV_CC_ConvertPixelType(pUser, &stParam);
        if (MV_OK != nRet)
        {
            printf("ConvertPixelType Fail: [0x%x]\n", nRet);
        }

        Mat img;
        Convert2Mat(&stParam, stParam.pDstBuffer, img);

        // run detector on provided image
        detector.DetectLive(img);


        // show images from live camera feed - this works
        imshow("cam", img);
        if (waitKey(5) >= 0) {
            break;
        }
        
        // flush buffer before next acquisition
        nRet = MV_CC_FreeImageBuffer(handle, &stOutFrame);
        if (nRet != MV_OK) {
            printf("Free Image Buffer fail! nRet [0x%x]\n", nRet);
        }
    }

    free(pFrameBuf);
    pFrameBuf = NULL;

    //Stop image acquisition
    nRet = MV_CC_StopGrabbing(handle);
    if (MV_OK != nRet)
    {
        printf("error: StopGrabbing fail [%x]\n", nRet);
        return;
    }

    //Close device, and release resource
    nRet = MV_CC_CloseDevice(handle);
    if (MV_OK != nRet)
    {
        printf("error: CloseDevice fail [%x]\n", nRet);
        return;
    }

    //Destroy handle and release resource
    nRet = MV_CC_DestroyHandle(handle);
    if (MV_OK != nRet)
    {
        printf("error: DestroyHandle fail [%x]\n", nRet);
        return;
    }
}

// getting live feed from the camera
void Camera::DisplayFeed(void* pUser) {
    MV_FRAME_OUT stOutFrame;
    memset(&stOutFrame, 0, sizeof(MV_FRAME_OUT));
    MV_DISPLAY_FRAME_INFO stDisplayOneFrame;
    memset(&stDisplayOneFrame, 0, sizeof(MV_DISPLAY_FRAME_INFO));
    int nTestFrameSize = 0;
    int nRet = -1;

    while (1)
    {
        if (nTestFrameSize > 99)
        {
            break;
        }
        ret = MV_CC_GetOneFrameTimeout(pUser, pFrameBuf, nBufSize, &stInfo, 1000);

        if (MV_OK != nRet)
        {
            Sleep(10);
        }
        else
        {
            //...Process image data
            nTestFrameSize++;
        }
        HMODULE hKernel32 = GetModuleHandle(L"kernel32");
        GetConsoleWindowAPI = (PROCGETCONSOLEWINDOW)GetProcAddress(hKernel32, "GetConsoleWindow");
        HWND hWnd = GetConsoleWindowAPI(); //window handle

        stDisplayOneFrame.hWnd = hWnd;
        unsigned char* garbo = (unsigned char*)malloc(sizeof(char) * 100);
        stDisplayOneFrame.pData = pFrameBuf;
        stDisplayOneFrame.nDataLen = stInfo.nFrameLen;
        stDisplayOneFrame.nWidth = stInfo.nWidth;
        stDisplayOneFrame.nHeight = stInfo.nHeight;
        stDisplayOneFrame.enPixelType = stInfo.enPixelType;

        nRet = MV_CC_DisplayOneFrame(handle, &stDisplayOneFrame);
        if (nRet != MV_OK) {
            printf("Display one frame fail! nRet [0x%x]\n", nRet);
        }

        nRet = MV_CC_FreeImageBuffer(handle, &stOutFrame);
        if (nRet != MV_OK) {
            printf("Free Image Buffer fail! nRet [0x%x]\n", nRet);
        }
    }
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

int RGB2BGR(unsigned char* pRgbData, unsigned int nWidth, unsigned int nHeight)
{
    if (NULL == pRgbData)
    {
        return MV_E_PARAMETER;
    }

    for (unsigned int j = 0; j < nHeight; j++)
    {
        for (unsigned int i = 0; i < nWidth; i++)
        {
            unsigned char red = pRgbData[j * (nWidth * 3) + i * 3];
            pRgbData[j * (nWidth * 3) + i * 3] = pRgbData[j * (nWidth * 3) + i * 3 + 2];
            pRgbData[j * (nWidth * 3) + i * 3 + 2] = red;
        }
    }

    return MV_OK;
}
bool Convert2Mat(MV_CC_PIXEL_CONVERT_PARAM* pstImageInfo, unsigned char* pData, cv::Mat& srcImage)
{
    if (pstImageInfo->enDstPixelType == PixelType_Gvsp_Mono8)
    {
        cout << "PixelType = PixelType_Gvsp_Mono8" << std::endl;
        srcImage = cv::Mat(pstImageInfo->nHeight, pstImageInfo->nWidth, CV_8UC1, pData);
    }
    else if (pstImageInfo->enDstPixelType == PixelType_Gvsp_RGB8_Packed)
    {
        RGB2BGR(pData, pstImageInfo->nWidth, pstImageInfo->nHeight);
        srcImage = cv::Mat(pstImageInfo->nHeight, pstImageInfo->nWidth, CV_8UC3, pData);
    }
    else
    {
        printf("unsupported pixel format\n");
        return false;
    }

    if (NULL == srcImage.data)
    {
        return false;
    }

    return true;
}