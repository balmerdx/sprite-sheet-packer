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
    , mask(other.mask.clone())
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
    mask = std::move(src.mask);
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

static QJsonObject toJson(QRect rect)
{
    QJsonObject json;
    json.insert("x", rect.left());
    json.insert("y", rect.top());
    json.insert("w", rect.width());
    json.insert("h", rect.height());
    return json;
}

static QJsonObject toJson(QPoint p)
{
    QJsonObject json;
    json.insert("x", p.x());
    json.insert("y", p.y());
    return json;
}

static QRect fromJson(const QJsonValue& v)
{
    const QJsonObject o = v.toObject();
    return QRect(o.value("x").toInt(), o.value("y").toInt(), o.value("w").toInt(), o.value("h").toInt());
}

bool PolygonPackContent::save(QDir storeDir)
{
    QString fullPath = storeDir.absoluteFilePath(_content.name());
    QFileInfo fullPathInfo(fullPath);
    fullPathInfo.dir().mkpath(".");

    //Здесь записать всю информацию PolygonPackContent
    if(!_content.image().save(fullPath))
    {
        qCritical() << "Cannot save image : " << fullPath;
        return false;
    }
    QString jsonPath = fullPathInfo.absoluteDir().absoluteFilePath(fullPathInfo.baseName()+".json");

    QJsonObject jsonRoot;
    jsonRoot.insert("name", _content.name());
    jsonRoot.insert("rect", toJson(_content.rect()));
    jsonRoot.insert("triangles", _content.triangles().toJson());
    jsonRoot.insert("mask", mask.toJson());
    jsonRoot.insert("offset", toJson(_offset));
    jsonRoot.insert("initial_bound", toJson(_initial_bound));
    jsonRoot.insert("area", _area);
    jsonRoot.insert("bounds", toJson(_bounds));

    QFile file(jsonPath);
    file.open(QIODevice::WriteOnly);
    if (!file.isOpen())
    {
        qCritical() << "Cannot save file :" << jsonPath;
        return false;
    }

    QJsonDocument jsonDoc(jsonRoot);
    file.write(jsonDoc.toJson(QJsonDocument::Indented));
    return file.error() == QFileDevice::NoError;
}

bool PackContent::load(QDir storeDir, QString name)
{
    QString fullPath = storeDir.absoluteFilePath(name);
    if(!_image.load(fullPath))
    {
        qCritical() << "Cannot load image : " << fullPath;
        return false;
    }

    QFileInfo fullPathInfo(fullPath);
    QString jsonPath = fullPathInfo.absoluteDir().absoluteFilePath(fullPathInfo.baseName()+".json");
    QFile file(jsonPath);
    file.open(QIODevice::ReadOnly);
    if (!file.isOpen())
    {
        qCritical() << "Cannot open file :" << jsonPath;
        return false;
    }

    QJsonDocument jsonDoc = QJsonDocument::fromJson(file.readAll());
    QJsonObject jsonRoot = jsonDoc.object();
    _name = jsonRoot.value("name").toString();
    if (_name != name)
    {
        qCritical() << "PackContent::load" << name << "!=" << _name;
        return false;
    }

    _rect = fromJson(jsonRoot.value("rect"));
    _triangles.fromJson(jsonRoot.value("triangles").toObject());
    return true;
}

///////////////////////PolygonPackBalmer//////////////////////////////////////////
PolygonPackBalmer::PolygonPackBalmer(bool verbose)
    : _verbose(verbose)
{
}

void PolygonPackBalmer::setContent(const std::vector<PackContent>& contents)
{
    _contentList.reserve(contents.size());
    for(const PackContent& content : contents)
    {
        _contentList.emplace_back(content);
    }

    for(PolygonPackContent& content : _contentList)
    {
        AImage cur_aimage_no_border(content.triangles().drawTriangles());
        AImage cur_aimage = cur_aimage_no_border.expandRightBottom(_spriteBorder);

        BinImage cur_image(cur_aimage, _granularity.width(), _granularity.height());

        content.pixel_border = std::move(cur_aimage);
        content.pixel_border.excludeMask(cur_aimage_no_border);
        content.mask = std::move(cur_image);
    }
}

void PolygonPackBalmer::place()
{

    std::sort(_contentList.begin(), _contentList.end(), [](const PolygonPackContent& a, const PolygonPackContent& b)
    {
        auto areaA = a.area();
        auto areaB = b.area();
        return areaA > areaB;
    });

    BinImage big_image(_maxTextureSize.width() / _granularity.width(),
                       _maxTextureSize.height() / _granularity.height());

    bool first = true;
    int idx = 0;
    for(PolygonPackContent& content : _contentList)
    {
        BinImageTest cur_image_test(content.mask);

        bool placed = false;
        for(int y=0; y<big_image.height() - content.mask.height(); y++)
        {
            for(int x=0; x<big_image.width() - content.mask.width(); x++)
            {
                if(cur_image_test.test(big_image, x, y))
                {
                    cur_image_test.place(big_image, x, y);
                    content.setOffsetNoMoveTriangle(QPoint(x * _granularity.width(), y * _granularity.height()));
                    if (first)
                    {
                        _bounds = content.bounds();
                        first = false;
                    } else
                    {
                        _bounds |= content.bounds();
                    }

                    if(_verbose)
                        qDebug() << idx << "/" << _contentList.size() << ":" << content.content().name();
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
}
