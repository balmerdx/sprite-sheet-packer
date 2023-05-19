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
    SpriteAtlas(const QStringList& sourceList = QStringList(),
                int textureBorder = 0,
                int spriteBorder = 1,
                int trim = 1,
                bool heuristicMask = false,
                bool pow2 = false,
                bool forceSquared = false,
                QSize maxSize = QSize(8192, 8192),
                float scale = 1);

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
};

extern bool verbose;

#endif // SPRITEATLAS_H
