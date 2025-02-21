#include "ElapsedTimer.h"

ElapsedTimer::ElapsedTimer(QObject* parent): QTimer(parent) {
    connect(this, &QTimer::timeout, [this](){
        emit timeout(elapsed());
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

int ElapsedTimer::elapsed() {
    auto elapsed_time = std::chrono::high_resolution_clock::now() - init_time_point;
    return std::chrono::duration_cast<std::chrono::microseconds>(elapsed_time).count();;
}

