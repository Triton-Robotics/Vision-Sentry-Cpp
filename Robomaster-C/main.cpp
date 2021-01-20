#include <thread>
#include "camera.h"
#include "detector.h"
#include "util.h"
#include "MvCameraControl.h"

using namespace cv;
using namespace std;


int main() {

    //Camera cam = Camera("sample_multiple.jpg");
    Camera cam = Camera();
    thread camera_thread;
    if (cam.input == 0) {
        // change function callback to function to test threading on
        camera_thread = thread(&Camera::DisplayFeed, cam, cam.handle);
    }
    else {
        camera_thread = thread(&Camera::DummyWorkThread, cam);
    }

    ////cv::Mat* addr = cam.GetAddress();
    ////Detector& detector = Detector::get(addr);
    ////runFPS(detector.Detect, 100);

    camera_thread.join();
    // cam.WorkThread();
    //return 0;
    //cam.WorkThread(&cam);
    cam.DisplayFeed(&cam);
    return 0;
}

    