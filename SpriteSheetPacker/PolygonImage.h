#ifndef POLYGONIMAGE_H
#define POLYGONIMAGE_H
#include "triangles.h"

class PolygonImage
{
public:
    PolygonImage(const QImage& image, const QRectF& rect, const float epsilon = 2.f, const float threshold = 0.05f);

    const Triangles& triangles() const { return _triangles; }
protected:
    const Polygons& polygons() const { return _polygons; }

protected:
    std::vector<QPointF> trace(QRectF rect, float threshold);
    QPair<bool, QPointF> findFirstNoneTransparentPixel(QRectF rect, float threshold);

    unsigned char getAlphaByIndex(const unsigned int& i);
    unsigned char getAlphaByPos(const QPointF& pos);
    int getIndexFromPos(int x, int y)
    {
        Q_ASSERT( x >= 0 && y >= 0 );
        return y*_width+x;
    }

    QPointF getPosFromIndex(int i)
    {
        Q_ASSERT( i >= 0);
        return QPointF(i % _width, i / _width);
    }

    unsigned int getSquareValue(int x, int y, QRectF rect, float threshold);
    std::vector<QPointF> marchSquare(QRectF rect, QPointF start, float threshold);
    float perpendicularDistance(const QPointF& i, const QPointF& start, const QPointF& end);
    std::vector<QPointF> rdp(std::vector<QPointF> v, const float& optimization);
    std::vector<QPointF> reduce(const std::vector<QPointF>& points, const QRectF& rect, const float& epsilon);
    std::vector<QPointF> expand(const std::vector<QPointF>& points, const QRectF& rect, const float& epsilon);
    bool combine(std::vector<QPointF>& a, const std::vector<QPointF>& b, const QRectF& rect, const float& epsilon);

    Triangles triangulate(const std::vector<QPointF>& points);

private:
    QImage        _image;
    unsigned int  _width;
    unsigned int  _height;
    unsigned int  _threshold;

    // out
    Triangles     _triangles;
    Polygons      _polygons;
};

#endif // POLYGONIMAGE_H
