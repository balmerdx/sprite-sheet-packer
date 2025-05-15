#include "ElapsedTimer.h"

ElapsedTimer::ElapsedTimer(QObject* parent): QTimer(parent) {
    connect(this, &QTimer::timeout, [this](){
        emit timeout(elapsed_us());
        init_time_point = std::chrono::high_resolution_clock::now();
    });
}

void ElapsedTimer::start() {
    init_time_point = std::chrono::high_resolution_clock::now();
    QTimer::start();
}

void ElapsedTimer::start(int msec) {
    init_time_point = std::chrono::high_resolution_clock::now();
    QTimer::start(msec);
}

double ElapsedTimer::elapsed_sec() {
    auto elapsed_time = std::chrono::high_resolution_clock::now() - init_time_point;
    return std::chrono::duration_cast<std::chrono::microseconds>(elapsed_time).count() * 1e-6;
}


int ElapsedTimer::elapsed_us() {
    auto elapsed_time = std::chrono::high_resolution_clock::now() - init_time_point;
    return std::chrono::duration_cast<std::chrono::microseconds>(elapsed_time).count();
}

