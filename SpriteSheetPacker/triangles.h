#pragma once

#include <QtCore>
#include <QtGui>
#include <stdint.h>

struct Triangles {
    /**Vertex data pointer.*/
    QVector<QPoint> verts;
    /**Index data pointer.*/
    QVector<uint16_t> indices;

    std::vector<QPointF> debugPoints;
    std::vector<int> debugPartInfo;

    void add(const Triangles& other) {
        uint16_t idx = static_cast<uint16_t>(verts.size());
        verts += other.verts;
        for (int i=0; i<other.indices.size(); ++i) {
            indices.push_back(other.indices[i] + idx);
        }
        debugPartInfo.push_back(other.indices.size() / 3);
    }

    //Предполагаем, что QPoint имеют неотрицательные значения и расположенны недалеко от нуля.
    QImage drawTriangles() const;

    QJsonObject toJson() const;
    void fromJson(const QJsonObject& jobj);
};

typedef std::vector<std::vector<QPointF>> Polygons;
