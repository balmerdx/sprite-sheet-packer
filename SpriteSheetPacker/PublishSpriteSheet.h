#ifndef PUBLISHSPRITESHEET_H
#define PUBLISHSPRITESHEET_H

#include <QtCore>
#include <QJSEngine>
#include <QtConcurrent>
#include "ImageFormat.h"
#include "PngOptimizer.h"
#include "SpriteAtlas.h"

struct ScalingVariant;

class JSConsole : public QObject {
    Q_OBJECT
public:
    explicit JSConsole() { }

public slots:
    void log(QString msg);
};


class PublishSpriteSheet: public QObject {
    Q_OBJECT

public:
    PublishSpriteSheet();

    void addSpriteSheet(const SpriteAtlas& atlas, const QString& fileName);
    void setImageFormat(ImageFormat imageFormat) { _imageFormat = imageFormat; }
    void setPixelFormat(PixelFormat pixelFormat) { _pixelFormat = pixelFormat; }
    void setPremultiplied(bool premultiplied) { _premultiplied = premultiplied; }
    void setPngQuality(const QString& optMode, int optLevel) { _pngQuality.optMode = optMode; _pngQuality.optLevel = optLevel; }
    void setWebpQuality(int quality) { _webpQuality = quality; }
    void setJpgQuality(int quality) { _jpgQuality = quality; }
    void setTrimSpriteNames(bool trimSpriteNames) { _trimSpriteNames = trimSpriteNames; }
    void setPrependSmartFolderName(bool prependSmartFolderName) { _prependSmartFolderName = prependSmartFolderName; }
    void setEncryptionKey(const QString& key) { _encryptionKey = key; }
    void setEnableSaveImage(bool e) { _enableSaveImage = e; }

    bool publish(const QString& format, bool errorMessage = true);

    bool notFitInOneTexture() const;


    static void addFormat(const QString& format, const QString& scriptFileName) { _formats[format] = scriptFileName; }
    static QMap<QString, QString>& formats() { return _formats; }

signals:
    void onCompletedOptimizePNG();

protected:
    bool generateDataFile(const QString& filePath, const QString& format, const QMap<QString, SpriteFrameInfo>& spriteFrames, const QImage& atlasImage, bool errorMessage = true);
    bool generateDataFileBinMask(const QString& filePath, const QMap<QString, SpriteFrameInfo>& spriteFrames);
    bool optimizePNG(const QString& fileName, const QString& optMode, int optLevel);
    void optimizePNGInThread(QStringList fileNames, const QString& optMode, int optLevel);

    bool saveImage(const QString& outputFilePath, const QImage& atlasImage);
protected:
    QFutureWatcher<bool> _watcher;
    QMutex _mutex;

    QList<SpriteAtlas> _spriteAtlases;
    QStringList _fileNames;

    ImageFormat _imageFormat = kPNG;
    PixelFormat _pixelFormat = kARGB8888;
    bool        _premultiplied = true;

    struct {
        QString optMode;
        int     optLevel;
    } _pngQuality;

    int _webpQuality = 80;
    int _jpgQuality = 80;

    bool _trimSpriteNames = true;
    bool _prependSmartFolderName = true;
    bool _enableSaveImage = true;

    QString     _encryptionKey;

    static QMap<QString, QString> _formats;
};

#endif // PUBLISHSPRITESHEET_H
