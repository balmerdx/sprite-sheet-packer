#include "triangles.h"
#include <QJsonObject>
#include <QJsonArray>

QImage Triangles::drawTriangles() const
{
    if(verts.empty())
        return QImage();

    QPoint pmax = verts[0];
    for(QPoint p : verts)
    {
        if (p.x() > pmax.x())
            pmax.setX(p.x());
        if (p.y() > pmax.y())
            pmax.setY(p.y());
    }

    QImage out(pmax.x(), pmax.y(), QImage::Format_ARGB32);

    {
        out.fill(QColor(0,0,0,0));
        QBrush brush(QColor(255, 255, 255));
        QPainter p(&out);
        p.setBrush(brush);
        p.setPen(Qt::NoPen);
        QList<QPoint> points;
        for(qsizetype i=0; i<indices.size(); i+=3)
        {
            points.clear();
            points.append(verts[indices[i+0]]);
            points.append(verts[indices[i+1]]);
            points.append(verts[indices[i+2]]);
            QPolygon poly(points);
            p.drawPolygon(poly);
        }
    }

    return std::move(out);
}

QJsonObject Triangles::toJson(QPoint offset) const
{
    QJsonObject out;

    QJsonArray arrVerts;
    for(QPoint p : verts)
    {
        arrVerts.push_back(p.x() + offset.x());
        arrVerts.push_back(p.y() + offset.y());
    }

    QJsonArray arrIndices;
    for(auto idx : indices)
    {
        arrIndices.push_back(idx);
    }


    out.insert("verts", arrVerts);
    out.insert("indices", arrIndices);
    return out;
}

void Triangles::fromJson(const QJsonObject& jobj)
{
    QJsonArray jverts = jobj.value("verts").toArray();
    QJsonArray jindices = jobj.value("indices").toArray();

    verts.resize(jverts.size() / 2);
    for(qsizetype i = 0; i < verts.size(); i++)
        verts[i] = QPoint(jverts[i*2].toInt(), jverts[i*2+1].toInt());

    indices.resize(jindices.size());
    for(qsizetype i = 0; i < jindices.size(); i++)
        indices[i] = jindices[i].toInt();

}
