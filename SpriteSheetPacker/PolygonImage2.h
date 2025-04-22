#pragma once
#include "triangles.h"

class PolygonImage2
{
public:
    PolygonImage2(const QImage& image, const QRectF& rect, const float epsilon = 2.f, const float threshold = 0.05f);

    const Triangles& triangles() const { return _triangles; }
private:
    QImage        _image;
    unsigned int  _width;
    unsigned int  _height;
    unsigned int  _threshold;

    // out
    Triangles     _triangles;
};

