#include "triangles.h"

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
