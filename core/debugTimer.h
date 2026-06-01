#pragma once

#include <chrono>
#include <string>
#include <iostream>
#include <cstdlib>

class Debug {
public:
    static void throwFatal(const char* title, const std::string& message) {
        std::cerr << "FATAL ERROR [" << title << "]: " << message << std::endl;
        std::abort();
    }

    static void beginTimeMeasure() {
        measureTimer = std::chrono::steady_clock::now();
    }

    static void endTimeMeasure() {
        auto now = std::chrono::steady_clock::now();
        double measureTime = std::chrono::duration<double, std::milli>(now - measureTimer).count();

        if (measureTime > maxMeasureTime) {
            maxMeasureTime = measureTime;
        }

        avgMeasureTime += (measureTime - lastMeasureTime) / static_cast<double>(measureFrames);
        lastMeasureTime = measureTime;
        measureFrames++;

        std::cout << "Measure: " << measureTime << "ms | Avg: " << avgMeasureTime
                  << "ms | Peak: " << maxMeasureTime << "ms" << std::endl;
    }

    static void resetTimeMeasure() {
        maxMeasureTime = 0.0;
        avgMeasureTime = 0.0;
        measureFrames = 1;
        lastMeasureTime = 0.0;
    }

private:
    static inline std::chrono::time_point<std::chrono::steady_clock> measureTimer;
    static inline double maxMeasureTime = 0.0;
    static inline double lastMeasureTime = 0.0;
    static inline double avgMeasureTime = 0.0;
    static inline size_t measureFrames = 1;
};