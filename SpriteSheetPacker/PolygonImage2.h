#pragma once
#include "triangles.h"

class PolygonImage2
{
public:
    PolygonImage2(const QImage& image, const QRectF& rect, bool packToRect);

    const Triangles& triangles() const { return _triangles; }
private:
    QImage        _image;
    unsigned int  _width;
    unsigned int  _height;

    // out
    Triangles     _triangles;
};

