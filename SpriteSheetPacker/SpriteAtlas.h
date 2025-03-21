#ifndef SPRITEATLAS_H
#define SPRITEATLAS_H

#include <QtCore>
#include <QImage>

#include "PolygonImage.h"

struct SpriteFrameInfo {
public:
    QRect   frame;
    QPoint  offset;
    bool    rotated;
    QRect   sourceColorRect;
    QSize   sourceSize;

    Triangles triangles;
};

class PackContent {
public:
    PackContent();
    PackContent(const QString& name, const QImage& image);

    bool isIdentical(const PackContent& other);
    void trim(int alpha);
    void setTriangles(const Triangles& triangles) { _triangles = triangles; }
    void setPolygons(const Polygons& polygons) { _polygons = polygons; }

    const QString& name() const { return _name; }
    const QImage& image() const { return _image; }
    const QRect& rect() const { return _rect; }
    const Triangles& triangles() const { return _triangles; }
    const Polygons& polygons() const { return _polygons; }

private:
    QString _name;
    QImage  _image;
    QRect   _rect;
    Triangles _triangles;
    Polygons  _polygons;
};

class SpriteAtlasGenerateProgress: public QObject
{
    Q_OBJECT
public:
    explicit SpriteAtlasGenerateProgress() { }
    ~SpriteAtlasGenerateProgress() {}

    void setProgressText(const QString& message) {
        emit progressTextChanged(message);
    }

signals:
    void progressTextChanged(const QString&);
};

class SpriteAtlas
{
public:
    struct OutputData {
        QImage _atlasImage;
        QMap<QString, SpriteFrameInfo> _spriteFrames;
    };

public:
    /*
       granularity - размер гранул теустуры.
       для RGBA текстур он равен (1,1), для ASTC4x4 он будет (4,4)
       Размещает спрайты так, что-бы они не пересекались в блоках granularity
       Это позволяет в ASTC4x4 копировать спрайты поотдельночти из одной текстуры в другую.
       В текущий момент реализовано только для rect упаковки.

       Предполагаем, что textureBorder=0, trim = 0, иначе кривовато работать будет
    */
    SpriteAtlas(const QStringList& sourceList = QStringList(),
                int textureBorder = 0,
                int spriteBorder = 1,
                int trim = 1,
                bool heuristicMask = false,
                bool pow2 = false,
                bool forceSquared = false,
                QSize maxSize = QSize(8192, 8192),
                float scale = 1,
                QSize granularity = QSize(1,1),
                QSize fixedTextureSize = QSize(0,0)
                );

    void setAlgorithm(const QString& algorithm) { _algorithm = algorithm; }
    void enablePolygonMode(bool enable, float epsilon = 2.f);

    //rotateCw = True - вращать спрайт по часовой стрелке.
    void setRotateSprites(bool value, bool rotateCw) { _rotateSprites = value; _rotateSpritesCw = rotateCw; }

    bool generate(SpriteAtlasGenerateProgress* progress = nullptr);
    void abortGeneration() { _aborted = true; }

    QString algorithm() const { return _algorithm; }
    float scale() const { return _scale; }

    void enableFindIdentical(bool enable) { _enableFindIdentical = enable; }

    const QVector<OutputData>& outputData() const { return _outputData; }
    const QMap<QString, QVector<QString>>& identicalFrames() const { return _identicalFrames; }

protected:
    bool packWithRect(const QVector<PackContent>& content);
    bool packWithPolygon(const QVector<PackContent>& content);

    void onPlaceCallback(int current, int count);

    int GranularityDivSizeX(int sx) { int gx = _granularity.width(); return (sx + gx - 1) / gx; }
    int GranularityDivSizeY(int sy) { int gy = _granularity.height(); return (sy + gy - 1) / gy; }

    int GranularityMulPosX(int x) { return x * _granularity.width(); }
    int GranularityMulPosY(int y) { return y * _granularity.height(); }

    int GranularityRoundUpX(int sx) { int gx = _granularity.width(); return (sx + gx - 1) / gx * gx; }
    int GranularityRoundUpY(int sy) { int gy = _granularity.height(); return (sy + gy - 1) / gy * gy; }
private:
    QStringList _sourceList;
    QString _algorithm = "Rect";
    int _trim;
    int _textureBorder;
    int _spriteBorder;
    bool _heuristicMask;
    bool _pow2;
    bool _forceSquared;
    QSize _maxTextureSize;
    QSize _fixedTextureSize;
    float _scale;
    bool _rotateSprites = false;
    bool _rotateSpritesCw = true;
    bool _enableFindIdentical = true;
    // polygon mode
    struct TPolygonMode{
        bool enable;
        float epsilon;
    } _polygonMode;

    SpriteAtlasGenerateProgress* _progress;

    // output data
    QVector<OutputData> _outputData;
    QMap<QString, QVector<QString>> _identicalFrames;

    bool _aborted = false;

    QSize _granularity = QSize(1,1);
};

extern bool verbose;

#endif // SPRITEATLAS_H
