#pragma once
#include "pack_content.h"
#include "bin_image.h"


class PolygonPackContent {
public:
    explicit PolygonPackContent(const PackContent& content);
    PolygonPackContent(const PolygonPackContent& other);
    PolygonPackContent(PolygonPackContent&& other);
    void operator=(PolygonPackContent&&);

    const PackContent& content() const { return _content; }
    double area() const { return _area; }
    const QPoint& offset() const { return _offset; }
    const QRect& bounds() const { return _bounds; }
    const Triangles& triangles() const { return _content.triangles(); }
    const QRect& initial_bound() const { return _initial_bound; }

    void setOffset(const QPoint& offset);
    void setOffsetNoMoveTriangle(const QPoint& offset);

    //Тестовое изображение. В нём ненулевые пикcели - это border
    AImage pixel_border;

    //Маска изображения, каждый бит отвечает за granularity область
    BinImage mask;
protected:
    PackContent _content;
    QPoint _offset;
    QRect _initial_bound;
    double _area;
    QRect _bounds;
};


class PolygonPackBalmer
{
public:
    PolygonPackBalmer(bool verbose);

    void place(const std::vector<PackContent>& contents, QSize maxTextureSize, QSize granularity, int border);

    const std::vector<PolygonPackContent>& contentList() const { return _contentList; }
    QRect bounds() const { return _bounds; }
protected:
    QRect _bounds;
    std::vector<PolygonPackContent> _contentList;
    bool _verbose;
};

