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
    // MVCC_INTVALUE stParam;
    MVCC_INTVALUE stIntvalue = { 0 };
    //memset(&stParam, 0, sizeof(MVCC_INTVALUE));
    // used to be &stParam for 3rd param
    ret = MV_CC_GetIntValue(handle, "PayloadSize", &stIntvalue);
    if (MV_OK != ret) {
        printf("Get PayloadSize fail! ret [0x%x]\n", ret);
    }
    // get nCurValue
    printf("nCurValue is: %d\n", stIntvalue.nCurValue);
    // buffer size - need to be made more efficient sizewise - was * 2 1.49 was too small  1.5 works 
    nBufSize = stIntvalue.nCurValue * 1.50; //One frame data size + reserved bytes (handled in SDK) was 2048
    //payload_size = stParam.nCurValue;

    // new code 

    unsigned int nTestFrameSize = 0;
    pFrameBuf = NULL;
    pFrameBuf = (unsigned char*)malloc(nBufSize);
    // test to see if pFrameBuf is overwritten during image acquisition
    //*pFrameBuf = 'c';

    memset(&stInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));

    // end new code

    // Start grab image
    ret = MV_CC_StartGrabbing(handle);
    if (MV_OK != ret) {
        printf("Start Grabbing fail! ret [0x%x]\n", ret);
    }

    //raw_data_ptr = (unsigned char*)malloc(sizeof(unsigned char) * (payload_size));
    //raw_data_size = payload_size;
    //image_info = { 0 };
    //memset(&image_info, 0, sizeof(MV_FRAME_OUT_INFO_EX));

    // test image receiving - first param was NULL - changed to handle
    //ret = MV_CC_GetOneFrameTimeout(handle, raw_data_ptr, raw_data_size, &image_info, 1000);
    //if (ret == MV_OK) {
    //    printf("GetFrame Test Passed.");
    //}
    //else {
    //    printf("GetFrame Test Failed.");
    //}

    // initialize RGB memory
    //rgb_data_ptr = (unsigned char*)malloc(image_info.nWidth * image_info.nHeight * 3);
    //if (NULL == rgb_data_ptr) {
    //    printf("malloc rgb_data_ptr fail!");
    //}
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

//void Camera::WorkThread(void* pUser)
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

    //stOutFrame.pBufAddr = pFrameBuf;
    // end new code

    unsigned char* pImage = (unsigned char*)malloc(nBufSize);

    int frame_num = 0;
    while (1)
    {
        //if (nTestFrameSize > 99)
        //{
        //    break;
        //}
        ////nRet = MV_CC_GetImageBuffer(handle, &stOutFrame, 1000);
        
        // test pFrameBuf init inside loop for memory leaks
   /*     pFrameBuf = NULL;
        pFrameBuf = (unsigned char*)malloc(nBufSize);*/

        //ret = MV_CC_GetOneFrameTimeout(pUser, pFrameBuf, nBufSize, &stInfo, 1000);
        nRet = MV_CC_GetImageBuffer(handle, &stOutFrame, 1000);
        if (MV_OK != nRet)
        {
            printf("Jank did not work\n");
        }
        //printf("Address after getImageBuffer: %p\n", stOutFrame.pBufAddr);

        //printf("pBuf: %s\n", pFrameBuf);
        ////printf("Get One Frame: Width[%d], Height[%d], nFrameNum[%d]\n",
        ////stInfo.nWidth, stInfo.nHeight, stInfo.nFrameNum);
        ////printf("frame buffer: %d\n", strlen((const char*)pFrameBuf));
        ////printf("buffer specified size: %d\n", nBufSize);
        //if (MV_OK != nRet)
        //{
        //    Sleep(10);
        //}
        //else
        //{
        //    //...Process image data
        //    nTestFrameSize++;
        //}
        //HMODULE hKernel32 = GetModuleHandle(L"kernel32");
        //GetConsoleWindowAPI = (PROCGETCONSOLEWINDOW)GetProcAddress(hKernel32, "GetConsoleWindow");
        //HWND hWnd = GetConsoleWindowAPI(); //window handle

        //stDisplayOneFrame.hWnd = hWnd;
        ////_MV_FRAME_OUT_INFO_EX_ temp = stOutFrame.stFrameInfo;
        //unsigned char* garbo = (unsigned char*)malloc(sizeof(char) * 100);
        //stDisplayOneFrame.pData = pFrameBuf;        
        //stDisplayOneFrame.nDataLen = stInfo.nFrameLen;
        //stDisplayOneFrame.nWidth = stInfo.nWidth;
        //stDisplayOneFrame.nHeight = stInfo.nHeight;        
        //stDisplayOneFrame.enPixelType = stInfo.enPixelType;
        //stDisplayOneFrame.enPixelType = PixelType_Gvsp_RGB8_Packed;

        //stInfo.enPixelType = PixelType_Gvsp_RGB8_Packed;
        
        //Transform input and output parameter to pixel format.
        //stOutFrame.stFrameInfo = stInfo;

        ////Source data
        //stParam.pSrcData = pFrameBuf;              //Original image data - is pFrameBuf
        //stParam.nSrcDataLen = stInfo.nFrameLen;         //Length of original image data
        //stParam.enSrcPixelType = stInfo.enPixelType;       //Pixel format of original image
        //stParam.nWidth = stInfo.nWidth;            //Image width
        //stParam.nHeight = stInfo.nHeight;           //Image height

        ////Target data
        //stParam.enDstPixelType = PixelType_Gvsp_RGB8_Packed;     //Pixel format type needs to be saved, it will transform to MONO8 format
        //stParam.nDstBufferSize = nBufSize;             //Size of storage node
        //unsigned char* pImage = (unsigned char*)malloc(nBufSize);
        //stParam.pDstBuffer = pImage;                   //Buffer for the output data,used to save the transformed data.

                //Source data
        stParam.pSrcData = stOutFrame.pBufAddr;              //Original image data - is pFrameBuf
        stParam.nSrcDataLen = stOutFrame.stFrameInfo.nFrameLen;         //Length of original image data
        stParam.enSrcPixelType = stOutFrame.stFrameInfo.enPixelType;       //Pixel format of original image
        stParam.nWidth = stOutFrame.stFrameInfo.nWidth;            //Image width
        stParam.nHeight = stOutFrame.stFrameInfo.nHeight;           //Image height

        //Target data
        stParam.enDstPixelType = PixelType_Gvsp_RGB8_Packed;     //Pixel format type needs to be saved, it will transform to MONO8 format
        stParam.nDstBufferSize = nBufSize;             //Size of storage node
        stParam.pDstBuffer = pImage;                   //Buffer for the output data,used to save the transformed data.

        nRet = MV_CC_ConvertPixelType(pUser, &stParam);
        if (MV_OK != nRet)
        {
            printf("ConvertPixelType Fail: [0x%x]\n", nRet);
        }

        Mat img;
        Convert2Mat(&stParam, stParam.pDstBuffer, img);
        //imshow("Result", img);
        string img_name = "result";
        string extension = ".png";

        // uncomment to see saved results
        //imwrite(img_name + to_string(frame_num) + extension, img);

        // run detector on provided image
        detector.DetectLive(img);


        // show images from live camera feed - this works
        imshow("cam", img);
        if (waitKey(5) >= 0) {
            break;
        }

        //printf("Address before free: %p\n", stOutFrame.pBufAddr);
        nRet = MV_CC_FreeImageBuffer(handle, &stOutFrame);
        if (nRet != MV_OK) {
            printf("Free Image Buffer fail! nRet [0x%x]\n", nRet);
        }
        //free(stOutFrame.pBufAddr);

        // TODO - failing here!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        //delete [] stOutFrame.pBufAddr;
        //stOutFrame.pBufAddr = 0;
        //printf("after delete\n");
        //stOutFrame.pBufAddr = NULL;
        //printf("Address after free: %x\n", stOutFrame.pBufAddr);


        //free(pFrameBuf);
        //pFrameBuf = NULL;

        //printf("DisplayOneFrame pData: %s\n", stDisplayOneFrame.pData);
        //nRet = MV_CC_DisplayOneFrame(handle, &stDisplayOneFrame);
        //if (nRet != MV_OK) {
        //    printf("Display one frame fail! nRet [0x%x]\n", nRet);
        //}

        //stOutFrame.stFrameInfo = temp;
        //stOutFrame.pBufAddr = pFrameBuf;
        //nRet = MV_CC_GetImageBuffer(handle, &stOutFrame, 1000);
        //printf("pFrameBuf after ImgBuffer: %s\n", pFrameBuf);
        //nRet = MV_CC_FreeImageBuffer(handle, &stOutFrame);
        //if (nRet != MV_OK) {
        //    printf("Free Image Buffer fail! nRet [0x%x]\n", nRet);
        //    //printf("Error code for nRet: %d\n", nRet);
        //}

        // uncomment to break after certain number of frames
        //if (frame_num++ > 100) {
        //    break;
        //}


        // actually try to convert image data into usable form
        //MV_CC_PIXEL_CONVERT_PARAM stConvertParam = { 0 };
        //memset(&stConvertParam, 0, sizeof(MV_CC_PIXEL_CONVERT_PARAM));

        //rgb_data_ptr = (unsigned char*)malloc(nBufSize);

        //stConvertParam.nWidth = stInfo.nWidth;                 //image width
        //stConvertParam.nHeight = stInfo.nHeight;               //image height
        //stConvertParam.pSrcData = pFrameBuf;                            //input data buffer
        //stConvertParam.nSrcDataLen = stInfo.nFrameLen;         //input data size
        //stConvertParam.enSrcPixelType = stInfo.enPixelType;    //input pixel format
        //stConvertParam.enDstPixelType = PixelType_Gvsp_RGB8_Packed; //coutput pixel format
        //stConvertParam.pDstBuffer = rgb_data_ptr;                    //output data buffer
        //stConvertParam.nDstBufferSize = nBufSize;            //output buffer size

        // ret = MV_CC_ConvertPixelType(handle, &stConvertParam);
        // if (MV_OK != ret) {
        //    printf("Convert Pixel Type fail! ret [0x%x]\n", ret);
        //    break;
        // }
    }


    // main loop
    //printf("pBuf before override: %d\n", strlen((const char*)pFrameBuf));
    //printf("pBuf character before override: %c\n", *pFrameBuf);
    //clock_t t0 = clock();
    //while (1) {
    //    clock_t t1 = clock();
    //    ret = MV_CC_GetOneFrameTimeout(pUser, pFrameBuf, nBufSize, &stInfo, 1000);

    //    if (ret == MV_OK) {
    //        printf("Get One Frame: Width[%d], Height[%d], nFrameNum[%d]\n",
    //            stInfo.nWidth, stInfo.nHeight, stInfo.nFrameNum);
    //        printf("Frame Buffer: %d\n", strlen((const char*)pFrameBuf));
    //        printf("Buffer specified size: %d\n", nBufSize);

            // Convert pixel format 
            // MV_CC_PIXEL_CONVERT_PARAM stConvertParam = { 0 };
            //memset(&stConvertParam, 0, sizeof(MV_CC_PIXEL_CONVERT_PARAM));

    //        //stConvertParam.nWidth = image_info.nWidth;                 //image width
    //        //stConvertParam.nHeight = image_info.nHeight;               //image height
    //        //stConvertParam.pSrcData = raw_data_ptr;                            //input data buffer
    //        //stConvertParam.nSrcDataLen = image_info.nFrameLen;         //input data size
    //        //stConvertParam.enSrcPixelType = image_info.enPixelType;    //input pixel format
    //        //stConvertParam.enDstPixelType = PixelType_Gvsp_RGB8_Packed; //coutput pixel format
    //        //stConvertParam.pDstBuffer = rgb_data_ptr;                    //output data buffer
    //        //stConvertParam.nDstBufferSize = rgb_data_size;            //output buffer size
    //        //// handle used to be pUser
    //        //ret = MV_CC_ConvertPixelType(handle, &stConvertParam);
    //        if (MV_OK != ret) {
    //            printf("Convert Pixel Type fail! ret [0x%x]\n", ret);
    //            break;
    //        }

    //        // new code
    //        //cv::Mat srcImage;
    //        ///*if (image_info.enPixelType == PixelType_Gvsp_Mono8)
    //        //{*/
    //        //for (unsigned int j = 0; j < image_info.nHeight; j++)
    //        //        {
    //        //            for (unsigned int i = 0; i < image_info.nWidth; i++)
    //        //            {
    //        //                unsigned char red = raw_data_ptr[j * (image_info.nWidth * 3) + i * 3];
    //        //                raw_data_ptr[j * (image_info.nWidth * 3) + i * 3] = raw_data_ptr[j * (image_info.nWidth * 3) + i * 3 + 2];
    //        //                raw_data_ptr[j * (image_info.nWidth * 3) + i * 3 + 2] = red;
    //        //            }
    //        //        }
    //        //    srcImage = cv::Mat(image_info.nHeight, image_info.nWidth, CV_8UC3, raw_data_ptr);
    //            //imshow("Display window", srcImage);
    //            //int k = waitKey(0);
    //            
    //        //}
    //        //else if (image_info.enPixelType == PixelType_Gvsp_RGB8_Packed)
    //        //{
    //        //    for (unsigned int j = 0; j < image_info.nHeight; j++)
    //        //    {
    //        //        for (unsigned int i = 0; i < image_info.nWidth; i++)
    //        //        {
    //        //            unsigned char red = raw_data_ptr[j * (image_info.nWidth * 3) + i * 3];
    //        //            raw_data_ptr[j * (image_info.nWidth * 3) + i * 3] = raw_data_ptr[j * (image_info.nWidth * 3) + i * 3 + 2];
    //        //            raw_data_ptr[j * (image_info.nWidth * 3) + i * 3 + 2] = red;
    //        //        }
    //        //    }
    //        //    //RGB2BGR(raw_data_ptr, image_info.nWidth, image_info.nHeight);
    //        //    srcImage = cv::Mat(image_info.nHeight, image_info.nWidth, CV_8UC3, raw_data_ptr);
    //        //}
    //        //else
    //        //{
    //        //    printf("unsupported pixel format\n");
    //        //}

    //        // added code
    //        //printf("here\n");
    //        //HMODULE hKernel32 = GetModuleHandle((LPCWSTR)"kernel32.dll");
    //        //GetConsoleWindowAPI = (PROCGETCONSOLEWINDOW)GetProcAddress(hKernel32, (LPCSTR)"GetConsoleWindow");
    //        //HWND hWnd = GetConsoleWindowAPI(); //window handle

    //        //////Display images
    //        //ret = MV_CC_Display(handle, hWnd);
    //        //if (ret != MV_OK) {
    //        //    printf("display frame fail!\n");
    //        //}


    //    }
    //    else {
    //        printf("No data[0x%x]\n", ret);
    //    }
    //}

    //free(raw_data_ptr);
    free(pFrameBuf);
    pFrameBuf = NULL;

    //free(detector);
    //detector = NULL;

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
        //nRet = MV_CC_GetImageBuffer(handle, &stOutFrame, 1000);
        ret = MV_CC_GetOneFrameTimeout(pUser, pFrameBuf, nBufSize, &stInfo, 1000);
        /*printf("pBuf: %s\n", pFrameBuf);*/
        //printf("Get One Frame: Width[%d], Height[%d], nFrameNum[%d]\n",
        //stInfo.nWidth, stInfo.nHeight, stInfo.nFrameNum);
        //printf("frame buffer: %d\n", strlen((const char*)pFrameBuf));
        //printf("buffer specified size: %d\n", nBufSize);
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
        //_MV_FRAME_OUT_INFO_EX_ temp = stOutFrame.stFrameInfo;
        unsigned char* garbo = (unsigned char*)malloc(sizeof(char) * 100);
        stDisplayOneFrame.pData = pFrameBuf;
        stDisplayOneFrame.nDataLen = stInfo.nFrameLen;
        stDisplayOneFrame.nWidth = stInfo.nWidth;
        stDisplayOneFrame.nHeight = stInfo.nHeight;
        stDisplayOneFrame.enPixelType = stInfo.enPixelType;
        //stDisplayOneFrame.enPixelType = PixelType_Gvsp_RGB8_Packed;

        //stInfo.enPixelType = PixelType_Gvsp_RGB8_Packed;
        //Mat img;
        //Convert2Mat(&stInfo, pFrameBuf, img);
        ////imshow("Result", img);
        //imwrite("result.png", img);
        //cvWaitKey(0);

        //printf("DisplayOneFrame pData: %s\n", stDisplayOneFrame.pData);
        nRet = MV_CC_DisplayOneFrame(handle, &stDisplayOneFrame);
        if (nRet != MV_OK) {
            printf("Display one frame fail! nRet [0x%x]\n", nRet);
        }

        //stOutFrame.stFrameInfo = temp;
        //stOutFrame.pBufAddr = pFrameBuf;
        //nRet = MV_CC_GetImageBuffer(handle, &stOutFrame, 1000);
        //printf("pFrameBuf after ImgBuffer: %s\n", pFrameBuf);
        nRet = MV_CC_FreeImageBuffer(handle, &stOutFrame);
        if (nRet != MV_OK) {
            printf("Free Image Buffer fail! nRet [0x%x]\n", nRet);
            //printf("Error code for nRet: %d\n", nRet);
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

    //save converted image in a local file
//     try {
// #if defined (VC9_COMPILE)
//         cvSaveImage("MatImage.bmp", &(IplImage(srcImage)));
// #else
//         cv::imwrite("MatImage.bmp", srcImage);
// #endif
//     }
//     catch (cv::Exception& ex) {
//         fprintf(stderr, "Exception saving image to bmp format: %s\n", ex.what());
//     }

//     srcImage.release();

    return true;
}