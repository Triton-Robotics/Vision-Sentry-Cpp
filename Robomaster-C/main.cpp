#include <thread>
#include "camera.h"
#include "detector.h"
#include "util.h"
#include "MvCameraControl.h"

using namespace cv;
using namespace std;


int main() {

    Camera cam = Camera();
    thread camera_thread;
    if (cam.input == 0) {
        // change function callback to function to test threading on
        camera_thread = thread(&Camera::WorkThread, cam, cam.handle);
    }
    else {
        camera_thread = thread(&Camera::DummyWorkThread, cam);
    }

    camera_thread.join();
 
    return 0;
}

    