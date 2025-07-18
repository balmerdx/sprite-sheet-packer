#include "PolygonImage2.h"
#include "image_border.h"
#include "optimize_by_clipper.h"

static void addTriangles(Triangles& triangles, const std::vector<p2t::Triangle*>& tris)
{
    for(std::vector<p2t::Triangle*>::const_iterator ite = tris.begin(); ite < tris.end(); ite++) {
        for(int i = 0; i < 3; i++) {
            auto p = (*ite)->GetPoint(i);
            auto v2 = QPoint(p->x, p->y);
            bool found = false;
            size_t j;
            size_t length = triangles.verts.size();
            for(j = 0; j < length; j++) {
                if(triangles.verts[j] == v2) {
                    found = true;
                    break;
                }
            }
            if(found) {
                //if we found the same vertex, don't add to verts, but use the same vertex with indices
            } else {
                j = triangles.verts.size();
                //vert does not exist yet, so we need to create a new one,
                triangles.verts.push_back(v2);
            }

            triangles.indices.push_back((uint16_t)j);
        }
    }

}

PolygonImage2::PolygonImage2(const QImage& image, const QRectF& rect, bool packToRect)
    : _width(image.width())
    , _height(image.height())
{
    _image = image.convertToFormat(QImage::Format_RGBA8888)
                 .copy(lroundf(rect.left()), lroundf(rect.top()), lroundf(rect.width()), lroundf(rect.height()));

    ImageBorder ib;
    ImageBorderParams params;

    if(packToRect)
    {
        ib.constructRect(_image);
    } else
    {
        ///!!!! params все параметры по умолчанию, ничего не взяли из настроек !!!!!
        ib.construct(_image, ImageBorderParams());
    }

    //Кривущая функция, если исходный вектор поменяется/удалится,
    //то указатели на точки станут невалидны
    auto makeTempPoints = [](const std::vector<p2t::Point>& p) ->std::vector<p2t::Point*>
    {
        std::vector<p2t::Point*> out(p.size());
        for(size_t i=0; i<p.size(); i++)
        {
            out[i] = const_cast<p2t::Point*>(&p[i]);
        }

        return std::move(out);
    };

    OptimizeByClipper op;
    op._params.pack_to_rect = packToRect;
    op.optimize(ib.elems);

    for(const ImageBorderElem& elem : op.result)
    {
        std::vector<p2t::Point*> pborder = makeTempPoints(elem.border);
        p2t::CDT cdt(pborder);

        for(auto& hole : elem.holes)
        {
            auto phole = makeTempPoints(hole);
            cdt.AddHole(phole);
        }

        cdt.Triangulate();
        std::vector<p2t::Triangle*> tris = cdt.GetTriangles();

        addTriangles(_triangles, tris);
    }
}

