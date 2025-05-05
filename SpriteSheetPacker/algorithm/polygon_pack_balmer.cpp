#include "polygon_pack_balmer.h"

PolygonPackContent::PolygonPackContent(const PackContent& content)
    : _content(content)
    , _area(0)
{
    auto& triangles = _content.triangles();
    if (triangles.verts.size() > 0)
    {
        _bounds.setTopLeft(triangles.verts[0]);
        _bounds.setBottomRight(triangles.verts[0]);
    }

    // calculate bounding box
    for (auto point: triangles.verts) {
        if (_bounds.left() > point.x()) _bounds.setLeft(point.x());
        if (_bounds.right() < point.x()) _bounds.setRight(point.x());
        if (_bounds.top() > point.y()) _bounds.setTop(point.y());
        if (_bounds.bottom() < point.y()) _bounds.setBottom(point.y());
    }

    _initial_bound = _bounds;
    setOffset(QPoint(-_bounds.left(), -_bounds.top()));
    _area = _bounds.width() * _bounds.height();
}

PolygonPackContent::PolygonPackContent(const PolygonPackContent& other)
    : _content(other._content)
    , _offset(other.offset())
    , _area(other._area)
    , _bounds(other._bounds)
    , _initial_bound(other._initial_bound)
    , pixel_border(other.pixel_border.clone())
{

}

PolygonPackContent::PolygonPackContent(PolygonPackContent&& other)
{
    *this = std::move(other);
}

void PolygonPackContent::operator=(PolygonPackContent&& src)
{
    _content = std::move(src._content);
    _offset = src.offset();
    _area = src._area;
    _bounds = src._bounds;
    _initial_bound = src._initial_bound;
    pixel_border = std::move(src.pixel_border);
}

void PolygonPackContent::setOffset(const QPoint& offset) {
    _offset = offset;
    _bounds.moveTo(_bounds.topLeft() + offset);

    // translate triangles
    auto& triangles = _content.triangles();
    for (QPoint& p : triangles.verts)
    {
        p += offset;
    }
}

void PolygonPackContent::setOffsetNoMoveTriangle(const QPoint& offset)
{
    _offset = offset;
    _bounds.moveTo(_bounds.topLeft() + offset);
}

PolygonPackBalmer::PolygonPackBalmer(bool verbose)
    : _verbose(verbose)
{
}

void PolygonPackBalmer::place(const std::vector<PackContent>& contents, QSize maxTextureSize, QSize granularity, int border)
{
    std::vector<PolygonPackContent> src_contents;
    src_contents.reserve(contents.size());
    for(const PackContent& content : contents)
    {
        src_contents.emplace_back(content);
    }

    std::sort(src_contents.begin(), src_contents.end(), [](const PolygonPackContent& a, const PolygonPackContent& b)
    {
        auto areaA = a.area();
        auto areaB = b.area();
        return areaA > areaB;
    });

    BinImage big_image(maxTextureSize.width() / granularity.width(), maxTextureSize.height() / granularity.height());

    bool first = true;
    int idx = 0;
    for(PolygonPackContent& content : src_contents)
    {
        AImage cur_aimage_no_border(content.triangles().drawTriangles());
        AImage cur_aimage = cur_aimage_no_border.expandRightBottom(border);
        content.pixel_border = cur_aimage.clone();
        content.pixel_border.excludeMask(cur_aimage_no_border);

        BinImage cur_image(cur_aimage, granularity.width(), granularity.height());
        BinImageTest cur_image_test(cur_image);

        bool placed = false;
        for(int y=0; y<big_image.height() - cur_image.height(); y++)
        {
            for(int x=0; x<big_image.width() - cur_image.width(); x++)
            {
                if(cur_image_test.test(big_image, x, y))
                {
                    cur_image_test.place(big_image, x, y);
                    content.setOffsetNoMoveTriangle(QPoint(x*granularity.width(), y*granularity.height()));
                    if (first)
                    {
                        _bounds = content.bounds();
                        first = false;
                    } else
                    {
                        _bounds |= content.bounds();
                    }

                    if(_verbose)
                        qDebug() << idx << "/" << src_contents.size() << ":" << content.content().name();
                    idx++;

                    placed = true;
                    break;
                }
            }

            if (placed)
                break;
        }

        //Место закончилось, картинки больше не размещаются
        if (!placed)
            break;
    }

    big_image.qimage().save("big_image.png");

    _contentList = std::move(src_contents);
}
