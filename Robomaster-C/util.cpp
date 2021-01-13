#include "util.h"
#include <iostream>
#include <chrono>

void runFPS(std::function<void()> f, int n)
{
    typedef std::chrono::high_resolution_clock clock;
    typedef std::chrono::duration<float, std::milli> duration;

    clock::time_point t0 = clock::now();
    std::vector<float> lapses;
    for (int i = n; i != 0; i--) {
        f();
        clock::time_point t1 = clock::now();
        duration elapsed = t1 - t0;
        auto i_millis = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
        float fps = 1000.0 / i_millis.count();
        printf("%.2f\n", fps);
        lapses.push_back(fps);
        t0 = t1;
    }
    std::sort(lapses.begin(), lapses.end());
    printf("Quartile FPS: %.2f, %.2f, %.2f\n", lapses[n / 4], lapses[n / 2], lapses[n / 4 * 3]);
}