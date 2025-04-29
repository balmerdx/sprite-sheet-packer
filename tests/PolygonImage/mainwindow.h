#pragma once

#include <QMainWindow>
#include <QMenuBar>
#include <QLabel>
#include <QHBoxLayout>
#include "image_border.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool load(QString filename);
    void addMenuElems();

    enum class DrawType
    {
        Triangle,
        Polygon,
        BinImage
    };

protected:
    void onSwitchDrawType(DrawType dt);
protected:
    QString filename;
    QLabel* labelImage = nullptr;
    QAction* actPolygon = nullptr;
    QAction* actTriangle = nullptr;
    QAction* actBinImage = nullptr;

    DrawType drawType = DrawType::BinImage;
};
