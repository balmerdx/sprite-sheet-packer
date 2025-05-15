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

    //Записывает внутри несколько файлов в поддиректории руководстуясь _content.name()
    //Читать будем толко часть внутри PackContent
    bool save(QDir storeDir);

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

    void setMaxTextureSize(QSize maxTextureSize) { _maxTextureSize = maxTextureSize; }
    void setGranularity(QSize granularity) { _granularity = granularity; }
    void setSpriteBorder(int spriteBorder) { _spriteBorder = spriteBorder; }
    void setContent(const std::vector<PackContent>& contents);

    void place();

    const std::vector<PolygonPackContent>& contentList() const { return _contentList; }
    QRect bounds() const { return _bounds; }
protected:
    QRect _bounds;
    std::vector<PolygonPackContent> _contentList;
    bool _verbose;

    QSize _maxTextureSize = QSize(1024, 1024);
    QSize _granularity = QSize(1,1);
    int _spriteBorder = 0;
};

