#pragma once
#include <QString>
#include "image_border.h"

struct TriangleVertices
{
    int x, y;
};

struct AtlasTriangle
{
    TriangleVertices v[3];

    double area();
};

struct AtlasStatsElem
{
    QString filename;
    //3 последовательно идущих индекса - это треугольник
    std::vector<int> indices;
    std::vector<TriangleVertices> vertices;

    std::vector<AtlasTriangle> triangles();
};

/*
   Загружаем атлас, рисуем полигоны на изображение.
   Считаем всякую статичтику.
*/
class AtlasStats
{
public:
    AtlasStats();
    ~AtlasStats();

    bool load(QString atlas_filename);

    void make_triangle_png(QString out_filename = "out.png");

    void save_areas(QString out_filename = "out.txt");
protected:
    QString atlas_filename;
    std::vector<AtlasStatsElem> atlas_elems;
};
