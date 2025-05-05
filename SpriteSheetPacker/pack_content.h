#pragma once
#include <QString>
#include <QImage>
#include "triangles.h"


class PackContent {
public:
    PackContent();
    PackContent(const QString& name, const QImage& image);

    bool isIdentical(const PackContent& other);

    //При полигональной упаковке собственный trim, который сильно более хороший и адаптивный
    //Этот надо вызывать позже, после того как полигональный trim почистил мусор.
    void trim(int alpha);
    void setTriangles(const Triangles& triangles) { _triangles = triangles; }

    const QString& name() const { return _name; }
    const QImage& image() const { return _image; }
    const QRect& rect() const { return _rect; }
    const Triangles& triangles() const { return _triangles; }
    Triangles& triangles() { return _triangles; }

private:
    QString _name;
    QImage  _image;
    QRect   _rect;
    Triangles _triangles;
};
