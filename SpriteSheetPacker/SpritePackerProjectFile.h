#ifndef SPRITEPACKERPROJECTFILE_H
#define SPRITEPACKERPROJECTFILE_H

#include <QtCore>
#include "ImageFormat.h"
#include "GenericObjectFactory.h"

struct ScalingVariant{
    QString name;
    float   scale;
    QSize   maxTextureSize;
    QSize   fixedTextureSize;//If not zero,  then this dimension is fixed
    bool    pow2;
    bool    forceSquared;
};

class SpritePackerProjectFile
{
public:
    SpritePackerProjectFile();
    virtual ~SpritePackerProjectFile();

    void setAlgorithm(const QString& algorithm) { _algorithm = algorithm; }
    QString algorithm() const { return _algorithm; }

    void setTrimMode(const QString& trimMode) { _trimMode = trimMode; }
    QString trimMode() const { return _trimMode; }

    void setTrimThreshold(int trimThreshold) { _trimThreshold = trimThreshold; }
    int trimThreshold() const { return _trimThreshold; }

    void setEpsilon(float epsilon) { _epsilon = epsilon; }
    float epsilon() const { return _epsilon; }

    void setHeuristicMask(bool heuristicMask) { _heuristicMask = heuristicMask; }
    bool heuristicMask() const { return _heuristicMask; }

    void setRotateSprites(bool rotate) { _rotateSprites = rotate; }
    bool rotateSprites() { return _rotateSprites; }
    bool rotateSpritesCw() { return rotateSpritesCwStatic(_dataFormat); }
    static bool rotateSpritesCwStatic(const QString& dataFormat);

    void setTextureBorder(int textureBorder) { _textureBorder = textureBorder; }
    int textureBorder() const { return _textureBorder; }

    void setSpriteBorder(int spriteBorder) { _spriteBorder = spriteBorder; }
    int spriteBorder() const { return _spriteBorder; }

    void setImageFormat(ImageFormat imageFormat) { _imageFormat = imageFormat; }
    ImageFormat imageFormat() const { return _imageFormat; }

    void setPixelFormat(PixelFormat pixelFormat) { _pixelFormat = pixelFormat; }
    PixelFormat pixelFormat() const { return _pixelFormat; }

    void setPremultiplied(bool premultiplied) { _premultiplied = premultiplied; }
    bool premultiplied() const { return _premultiplied; }

    void setPngOptMode(const QString& optMode) { _pngOptMode = optMode; }
    const QString& pngOptMode() const { return _pngOptMode; }

    void setPngOptLevel(int optLevel) { _pngOptLevel = optLevel; }
    int pngOptLevel() const { return _pngOptLevel; }

    void setWebpQuality(int quality) { _webpQuality = quality; }
    int webpQuality() const { return _webpQuality; }

    void setJpgQuality(int quality) { _jpgQuality = quality; }
    int jpgQuality() const { return _jpgQuality; }

    void setScalingVariants(const QVector<ScalingVariant>& scalingVariants) { _scalingVariants = scalingVariants; }
    const QVector<ScalingVariant>& scalingVariants() const { return _scalingVariants; }

    void setDataFormat(const QString& dataFormat) { _dataFormat = dataFormat; }
    const QString& dataFormat() const { return _dataFormat; }

    void setDestPath(const QString& destPath) { _destPath = destPath; }
    const QString& destPath() const { return _destPath; }

    void setSpriteSheetName(const QString& spriteSheetName) { _spriteSheetName = spriteSheetName; }
    const QString& spriteSheetName() const { return _spriteSheetName; }

    void setSrcList(const QStringList& srcList) { _srcList = srcList; }
    const QStringList& srcList() const { return _srcList; }

    void setTrimSpriteNames(bool trimSpriteNames) { _trimSpriteNames = trimSpriteNames; }
    bool trimSpriteNames() const { return _trimSpriteNames; }

    void setPrependSmartFolderName(bool prependSmartFolderName) { _prependSmartFolderName = prependSmartFolderName; }
    bool prependSmartFolderName() const { return _prependSmartFolderName; }

    void setEncryptionKey(const QString& key) { _encryptionKey = key; }
    const QString& encryptionKey() { return _encryptionKey; }

    void setEnableFindIdentical(bool e) { _enableFindIdentical = e; }
    bool enableFindIdentical() const { return _enableFindIdentical; }

    QSize granularity() const { return _granularity; }

    virtual bool write(const QString& fileName);
    virtual bool read(const QString& fileName);

    static GenericObjectFactory<std::string, SpritePackerProjectFile>& factory() {
        return _factory;
    }

    //Список изображений, которые не пакуются полигонально.
    //У них полигон будет прямоугольником
    const QStringList& getTrimRectList() const { return _trimRectListFiles; }
    void setTrimRectList(const QStringList& trimRectListFiles) { _trimRectListFiles = trimRectListFiles; }

    /*
       Функция, которая читает из файла listFilename находящегося в директории dir список файлов.
       Имена файлов являются относительными и начинаются с dir.
       Если такой файл не найден, то он игнорируется и пишется в лог.
       Разделители частей пути в unix style.
    */
    static bool loadFilesList(QDir dir, QString listFilename, QString tag, QStringList& filesList, bool checkFileExists=true);
protected:
    QString     _algorithm;
    QString     _trimMode;
    int         _trimThreshold;
    float       _epsilon;
    bool        _heuristicMask;
    bool        _rotateSprites;
    int         _textureBorder;
    int         _spriteBorder;
    ImageFormat _imageFormat;
    PixelFormat _pixelFormat;
    bool        _premultiplied;

    QString     _pngOptMode;
    int         _pngOptLevel;
    int         _webpQuality;
    int         _jpgQuality;

    QVector<ScalingVariant> _scalingVariants;

    QString     _dataFormat;
    QString     _destPath;
    QString     _spriteSheetName;
    QStringList _srcList;

    bool        _trimSpriteNames;
    bool        _prependSmartFolderName;
    bool        _enableFindIdentical = true;
    QString     _encryptionKey;

    QSize _granularity = QSize(1,1);
    QStringList _trimRectListFiles;
    QString _trimRectList;
private:
    static GenericObjectFactory<std::string, SpritePackerProjectFile> _factory;

    bool loadTrimRectList(QDir dir, QString trimRectList);
};

class SpritePackerProjectFileTPS: public SpritePackerProjectFile {
public:
    virtual bool write(const QString&) { return false; }
    virtual bool read(const QString& fileName);
};


#endif // SPRITEPACKERPROJECTFILE_H
