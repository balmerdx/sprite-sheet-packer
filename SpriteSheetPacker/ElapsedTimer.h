#ifndef ELAPSEDTIMER_H
#define ELAPSEDTIMER_H

#include <QtCore>
#include <chrono>

class ElapsedTimer: public QTimer
{
    Q_OBJECT
public:
    ElapsedTimer(QObject* parent);

    void start();
    void start(int msec);

    int elapsed();

signals:
    void timeout(int elapsed);

private:
    std::chrono::high_resolution_clock::time_point init_time_point;
};


#endif // ELAPSEDTIMER_H
