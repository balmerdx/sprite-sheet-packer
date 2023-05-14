#include "SpriteAtlas.h"

#include <functional>
#include "binpack2d.hpp"
#include "polypack2d.h"
#include "ImageRotate.h"
#include "PolygonImage.h"

int pow2(int len) {
    int order = 1;
    while(pow(2,order) < len)
    {
       order++;
    }
    return pow(2,order);
}

PackContent::PackContent() {
    // only for QVector
    qDebug() << "PackContent::PackContent()";
}
PackContent::PackContent(const QString& name, const QImage& image) {
    _name = name;
    _image = image;
    _rect = QRect(0, 0, _image.width(), _image.height());
}

bool PackContent::isIdentical(const PackContent& other) {
    if (_rect != other._rect) return false;

    for (int x = _rect.left(); x < _rect.right(); ++x) {
        for (int y = _rect.top(); y < _rect.bottom(); ++y) {
            if (_image.pixel(x, y) != other._image.pixel(x, y)) return false;
        }
    }

    return true;
}

void PackContent::trim(int alpha) {
    int l = _image.width();
    int t = _image.height();
    int r = 0;
    int b = 0;
    for (int y=0; y<_image.height(); y++) {
        bool rowFilled = false;
        for (int x=0; x<_image.width(); x++) {
            int a = qAlpha(_image.pixel(x, y));
            if (a >= alpha) {
                rowFilled = true;
                r = qMax(r, x);
                if (l > x) {
                    l = x;
                }
            }
        }
        if (rowFilled) {
            t = qMin(t, y);
            b = y;
        }
    }
    _rect = QRect(QPoint(l, t), QPoint(r,b));
    if ((_rect.width() % 2) != (_image.width() % 2)) {
        if (l>0) l--; else r++;
        _rect = QRect(QPoint(l, t), QPoint(r,b));
    }
    if ((_rect.height() % 2) != (_rect.height() % 2)) {
        if (t>0) t--; else b++;
        _rect = QRect(QPoint(l, t), QPoint(r,b));
    }
    if ((_rect.width()<0)||(_rect.height()<0)) {
        _rect = QRect(0, 0, 2, 2);
    }
}

SpriteAtlas::SpriteAtlas(const QStringList& sourceList, int textureBorder, int spriteBorder, int trim, bool heuristicMask, bool pow2, bool forceSquared,
                         QSize maxSize, float scale)
    : _sourceList(sourceList)
    , _trim(trim)
    , _textureBorder(textureBorder)
    , _spriteBorder(spriteBorder)
    , _heuristicMask(heuristicMask)
    , _pow2(pow2)
    , _forceSquared(forceSquared)
    , _maxTextureSize(maxSize)
    , _scale(scale)
{
    _polygonMode.enable = false;
}

void SpriteAtlas::enablePolygonMode(bool enable, float epsilon) {
    _polygonMode.enable = enable;
    _polygonMode.epsilon = epsilon;
}

bool SpriteAtlas::generate(SpriteAtlasGenerateProgress* progress) {
    _aborted = false;

    QTime timePerform;
    timePerform.start();

    _outputData.clear();

    _progress = progress;

    if (_progress)
        _progress->setProgressText(QString("Optimizing sprites..."));

    QStringList nameFilter;
    nameFilter << "*.png" << "*.jpg" << "*.jpeg" << "*.gif" << "*.bmp";

    QList< QPair<QString, QString> > fileList;
    for(auto pathName: _sourceList) {
        if (_aborted) return false;

        QFileInfo fi(pathName);

        if (fi.isDir()) {
            QDir dir(pathName);
            QDirIterator fileNames(pathName, nameFilter, QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while(fileNames.hasNext()){
                if (_aborted) return false;

                fileNames.next();
                QString filePath = fileNames.filePath();
                QString relativePath = dir.relativeFilePath(filePath);
                fileList.push_back(qMakePair(fileNames.filePath(), relativePath));
            }
        } else {
            fileList.push_back(qMakePair(pathName, fi.fileName()));
        }
    }

    int skipSprites = 0;

    // init images and rects
    _identicalFrames.clear();

    int progressIndex = 1;
    QVector<PackContent> inputContent;
    auto it_f = fileList.begin();
    for(; it_f != fileList.end(); ++it_f, ++progressIndex) {
        if (_aborted) return false;

        QImage image((*it_f).first);
        if (image.isNull()) continue;
        if (_scale != 1) {
            image = image.scaled(ceil(image.width() * _scale), ceil(image.height() * _scale), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        if (image.format() == QImage::Format_Indexed8) {
            image = image.convertToFormat(QImage::Format_ARGB32);
        }

        // Apply Heuristic mask
        if (_heuristicMask) {
            QPixmap pix = QPixmap::fromImage(image);
            pix.setMask(pix.createHeuristicMask());
            image = pix.toImage();
        }

        PackContent packContent((*it_f).second, image);

        // Trim / Crop
        if (_trim) {
            packContent.trim(_trim);
            if (_polygonMode.enable) {
                //qDebug() << (*it_f).first;
                PolygonImage polygonImage(packContent.image(), packContent.rect(), _polygonMode.epsilon, _trim);
                packContent.setPolygons(polygonImage.polygons());
                packContent.setTriangles(polygonImage.triangles());
            }
        }

        if(_enableFindIdentical)
        {
            // Find Identical
            bool findIdentical = false;
            for (auto& content: inputContent) {
                if (content.isIdentical(packContent)) {
                    findIdentical = true;
                    _identicalFrames[content.name()].push_back(packContent.name());
                    if(verbose)
                        qDebug() << "isIdentical:" << packContent.name() << "==" << content.name();
                    skipSprites++;
                    break;
                }
            }
            if (findIdentical) {
                continue;
            }
        }

        inputContent.push_back(packContent);
    }
    if (skipSprites)
        qDebug() << "Total skip sprites: " << skipSprites;

    bool result = false;
    if ((_algorithm == "Polygon") && (_polygonMode.enable)) {
        result = packWithPolygon(inputContent);
    } else {
        result = packWithRect(inputContent);
    }

    int elapsed = timePerform.elapsed();
    if(verbose)
        qDebug() << "Generate time mc:" <<  elapsed/1000.f << "sec";

    return result;
}

static bool tryPermutation(BinPack2D::ContentAccumulator<PackContent>& inputContent, BinPack2D::ContentAccumulator<PackContent>& outputContent, int w, int h,
                           BinPack2D::ContentAccumulator<PackContent>& remainder)
{
    //У нас максимально большие размеры текстуры. Попробуем первые самые большие спрайты вращать, может это поможет.
    bool success = false;
    uint32_t rotate_count = qMin((uint32_t)inputContent.Get().size(), (uint32_t)4);
    uint32_t rotate_exp2 = 1 << rotate_count;
    for(uint32_t try_idx = 1; try_idx < rotate_exp2; try_idx++)
    {
        for(uint32_t i = 0; i < rotate_count; i++)
        {
            if(!inputContent.Get()[i].tryRotate)
                return false;
            inputContent.Get()[i].setRotate((try_idx & (1<<i))?true:false);
        }

        qDebug() << "try_idx=" << try_idx;

        BinPack2D::CanvasArray<PackContent> canvasArray = BinPack2D::UniformCanvasArrayBuilder<PackContent>(w, h, 1).Build();
        success = canvasArray.Place(inputContent, remainder);
        if(success)
        {
            qDebug() << "try_idx success!!!!";
            outputContent = BinPack2D::ContentAccumulator<PackContent>();
            canvasArray.CollectContent(outputContent);
            break;
        }
    }

    return success;
}

bool SpriteAtlas::packWithRect(const QVector<PackContent>& content) {
    if (_progress)
        _progress->setProgressText("Optimizing atlas...");

    int volume = 0;
    BinPack2D::ContentAccumulator<PackContent> inputContent;
    for (auto packContent: content) {
        int width = packContent.rect().width();
        int height = packContent.rect().height();
        volume += width * height * 1.02f;

        inputContent += BinPack2D::Content<PackContent>(packContent,
                                                        BinPack2D::Coord(),
                                                        BinPack2D::Size(width + _spriteBorder, height + _spriteBorder),
                                                        _rotateSprites,
                                                        false);
    }

    // Sort the input content by size... usually packs better.
    inputContent.Sort();

    // A place to store packed content.
    BinPack2D::ContentAccumulator<PackContent> remainder;
    BinPack2D::ContentAccumulator<PackContent> outputContent;

    // find optimal size for atlas
    int w = qMin(_maxTextureSize.width(), (int)sqrt(volume));
    int h = qMin(_maxTextureSize.height(), (int)sqrt(volume));
    if (_forceSquared) {
        h = w;
    }
    if (_pow2) {
        w = pow2(w);
        h = pow2(h);
        qDebug() << "Volume size:" << w << "x" << h;

        bool k = true;
        while (1) {
            if (_aborted) return false;

            BinPack2D::CanvasArray<PackContent> canvasArray = BinPack2D::UniformCanvasArrayBuilder<PackContent>(w - _textureBorder*2, h - _textureBorder*2, 1).Build();

            bool success = canvasArray.Place(inputContent, remainder);
            if (success) {
                outputContent = BinPack2D::ContentAccumulator<PackContent>();
                canvasArray.CollectContent(outputContent);
                break;
            } else {
                if ((w == _maxTextureSize.width()) && (h == _maxTextureSize.height())) {
                    qDebug() << "Max size Limit!";
                    //typedef BinPack2D::Content<PackContent>::Vector::iterator binpack2d_iterator;
                    QVector<PackContent> remainderContent;
                    for (auto itor = remainder.Get().begin(); itor != remainder.Get().end(); itor++ ) {
                        const BinPack2D::Content<PackContent> &content = *itor;

                        const PackContent &packContent = content.content;
                        remainderContent.push_back(packContent);

                        qDebug() << packContent.name() << content.size.w << content.size.h;
                    }
                    qDebug() << "content:" << content.size();
                    qDebug() << "remainderContent:" << remainderContent.size();

                    outputContent = BinPack2D::ContentAccumulator<PackContent>();
                    canvasArray.CollectContent(outputContent);

                    packWithRect(remainderContent);

                    break;
                }
            }
            if (k || _forceSquared) {
                k = false;
                w = qMin(w*2, _maxTextureSize.width());
            } else {
                k = true;
                h = qMin(h*2, _maxTextureSize.height());
            }
            if (_forceSquared) {
                h = w;
            }

            if(verbose)
                qDebug() << "Resize for bigger:" << w << "x" << h;
        }
        while (w > 2) {
            if (_aborted) return false;

            w = w/2;
            if (_forceSquared) {
                h = w;
            }
            BinPack2D::CanvasArray<PackContent> canvasArray = BinPack2D::UniformCanvasArrayBuilder<PackContent>(w - _textureBorder*2, h - _textureBorder*2, 1).Build();

            bool success = canvasArray.Place(inputContent, remainder);
            if (!success) {
                w = w*2;
                if (_forceSquared) {
                    h = w;
                }
                break;
            } else {
                outputContent = BinPack2D::ContentAccumulator<PackContent>();
                canvasArray.CollectContent(outputContent);
            }

            if(verbose)
                qDebug() << "Optimize width:" << w << "x" << h;
        }
        if (!_forceSquared) {
            while (h > 2) {
                if (_aborted) return false;

                h = h/2;
                BinPack2D::CanvasArray<PackContent> canvasArray = BinPack2D::UniformCanvasArrayBuilder<PackContent>(w - _textureBorder*2, h - _textureBorder*2, 1).Build();

                bool success = canvasArray.Place(inputContent, remainder);
                if (!success) {
                    h = h*2;
                    break;
                } else {
                    outputContent = BinPack2D::ContentAccumulator<PackContent>();
                    canvasArray.CollectContent(outputContent);
                }

                if(verbose)
                    qDebug() << "Optimize height:" << w << "x" << h;
            }
        }
    } else {
        if(verbose)
            qDebug() << "Volume size:" << w << "x" << h;
        bool k = true;
        int step = qMax((w + h) / 20, 1);
        while (1) {
            if (_aborted) return false;

            BinPack2D::CanvasArray<PackContent> canvasArray = BinPack2D::UniformCanvasArrayBuilder<PackContent>(w - _textureBorder*2, h - _textureBorder*2, 1).Build();

            bool success = canvasArray.Place(inputContent, remainder);
            if (success) {
                outputContent = BinPack2D::ContentAccumulator<PackContent>();
                canvasArray.CollectContent(outputContent);
                break;
            } else {
                if ((w == _maxTextureSize.width()) && (h == _maxTextureSize.height())) {
                    qDebug() << "Max size Limit!";

                    if(tryPermutation(inputContent, outputContent, w - _textureBorder*2, h - _textureBorder*2,
                                      remainder))
                        break;

                    //typedef BinPack2D::Content<PackContent>::Vector::iterator binpack2d_iterator;
                    QVector<PackContent> remainderContent;
                    for (auto itor = remainder.Get().begin(); itor != remainder.Get().end(); itor++ ) {
                        const BinPack2D::Content<PackContent> &content = *itor;

                        const PackContent &packContent = content.content;
                        remainderContent.push_back(packContent);

                        qDebug() << packContent.name() << content.size.w << content.size.h;
                    }
                    qDebug() << "content:" << content.size();
                    qDebug() << "remainderContent:" << remainderContent.size();

                    if(outputContent.Get().size()==0)
                    {
                        //One element dont fit in texture limit!
                        return false;
                    }

                    outputContent = BinPack2D::ContentAccumulator<PackContent>();
                    canvasArray.CollectContent(outputContent);

                    packWithRect(remainderContent);

                    break;
                }
            }
            if (k || _forceSquared) {
                k = false;
                w = qMin(w + step, _maxTextureSize.width());
            } else {
                k = true;
                h = qMin(h + step, _maxTextureSize.height());
            }
            if (_forceSquared) {
                h = w;
            }

            if(verbose)
                qDebug() << "Resize for bigger:" << w << "x" << h << "step:" << step;
        }
        step = qMax((w + h) / 20, 1);
        while (w) {
            if (_aborted) return false;

            w -= step;
            if (_forceSquared) {
                h = w;
            }
            BinPack2D::CanvasArray<PackContent> canvasArray = BinPack2D::UniformCanvasArrayBuilder<PackContent>(w - _textureBorder*2, h - _textureBorder*2, 1).Build();

            bool success = canvasArray.Place(inputContent, remainder);
            if (!success) {
                w += step;
                if (_forceSquared) {
                    h = w;
                }
                if (step > 1) step = qMax(step/2, 1); else break;
            } else {
                outputContent = BinPack2D::ContentAccumulator<PackContent>();
                canvasArray.CollectContent(outputContent);
            }

            if(verbose)
                qDebug() << "Optimize width:" << w << "x" << h << "step:" << step;
        }
        if (!_forceSquared) {
            step = qMax((w + h) / 20, 1);
            while (h) {
                if (_aborted) return false;

                h -= step;
                BinPack2D::CanvasArray<PackContent> canvasArray = BinPack2D::UniformCanvasArrayBuilder<PackContent>(w - _textureBorder*2, h - _textureBorder*2, 1).Build();

                bool success = canvasArray.Place(inputContent, remainder);
                if (!success) {
                    h += step;
                    if (step > 1) step = qMax(step/2, 1); else break;
                } else {
                    outputContent = BinPack2D::ContentAccumulator<PackContent>();
                    canvasArray.CollectContent(outputContent);
                }

                if(verbose)
                    qDebug() << "Optimize height:" << w << "x" << h << "step:" << step;
            }
        }
    }

    if(verbose)
        qDebug() << "Found optimize size:" << w << "x" << h;
    if (_progress)
        _progress->setProgressText(QString("Found optimize size: %1x%2").arg(w).arg(h));

    OutputData outputData;

    // parse output.
    outputData._atlasImage = QImage(w, h, QImage::Format_RGBA8888);
    outputData._atlasImage.fill(QColor(0, 0, 0, 0));
    QPainter painter(&outputData._atlasImage);
    for(auto itor = outputContent.Get().begin(); itor != outputContent.Get().end(); itor++ ) {
        if (_aborted) return false;

        const BinPack2D::Content<PackContent> &content = *itor;

        // retreive your data.
        const PackContent &packContent = content.content;
        //qDebug() << packContent.mName << packContent.mRect;

        // image
        QImage image;
        if (content.rotated) {
            image = packContent.image().copy(packContent.rect());
            //Breaking changes!!! rotate 270 istead rotate 90
            image = rotate270(image);
        }

        SpriteFrameInfo spriteFrame;
        spriteFrame.triangles = packContent.triangles();
        spriteFrame.frame = QRect(content.coord.x + _textureBorder, content.coord.y + _textureBorder, content.size.w - _spriteBorder, content.size.h - _spriteBorder);
        if (spriteFrame.triangles.indices.size()) {
            spriteFrame.offset = QPoint(
                        packContent.rect().left(),
                        packContent.rect().top()
                        );
        } else {
            QSize imageSize = content.rotated
                    ? QSize(packContent.image().height(), packContent.image().width())
                    : QSize(packContent.image().width(), packContent.image().height());
            spriteFrame.offset = QPoint(
                        (packContent.rect().left() + (-imageSize.width() + content.size.w - _spriteBorder) * 0.5f),
                        (-packContent.rect().top() + ( imageSize.height() - content.size.h + _spriteBorder) * 0.5f)
                        );
        }
        spriteFrame.rotated = content.rotated;
        spriteFrame.sourceColorRect = packContent.rect();
        spriteFrame.sourceSize = packContent.image().size();
        if (content.rotated) {
            spriteFrame.frame = QRect(content.coord.x, content.coord.y, content.size.h-_spriteBorder, content.size.w-_spriteBorder);

        }
        if (content.rotated) {
            painter.drawImage(QPoint(content.coord.x + _textureBorder, content.coord.y + _textureBorder), image);
        } else {
            painter.drawImage(QPoint(content.coord.x + _textureBorder, content.coord.y + _textureBorder), packContent.image(), packContent.rect());
        }

        outputData._spriteFrames[packContent.name()] = spriteFrame;

        // add ident to sprite frames
        auto identicalIt = _identicalFrames.find(packContent.name());
        if (identicalIt != _identicalFrames.end()) {
            QStringList identicalList;
            for (auto ident: (*identicalIt)) {
                outputData._spriteFrames[ident] = spriteFrame;

                identicalList.push_back(ident);
            }
        }
    }

    painter.end();
    _outputData.push_front(outputData);

    return true;
}

bool SpriteAtlas::packWithPolygon(const QVector<PackContent>& content) {
    if (_progress)
        _progress->setProgressText("Build pack contents...");

    // initialize content
    PolyPack2D::ContentList<PackContent> inputContent;
    for (auto packContent: content) {
        //TODO: remove convert
        PolyPack2D::Triangles triangles;
        for (auto vert: packContent.triangles().verts) {
            triangles.verts.push_back(PolyPack2D::Point(vert.x(), vert.y()));
        }
        triangles.indices = packContent.triangles().indices.toStdVector();
        inputContent += PolyPack2D::Content<PackContent>(packContent, triangles, _spriteBorder);
    }

    // Sort the input content by area... usually packs better.
    inputContent.sort();

    for (auto it = inputContent.begin(); it != inputContent.end(); ++it) {
        qDebug() << (*it).content().name() << (*it).area();
    }

    PolyPack2D::Container<PackContent> container;
    // TODO: abort this place if _aborted
    container.place(inputContent, _maxTextureSize, 5, std::bind(&SpriteAtlas::onPlaceCallback, this, std::placeholders::_1, std::placeholders::_2));

    auto outputContent = container.contentList();

    OutputData outputData;

    outputData._atlasImage = QImage(container.bounds().width() + _textureBorder * 2, container.bounds().height() + _textureBorder * 2, QImage::Format_RGBA8888);
    outputData._atlasImage.fill(QColor(0, 0, 0, 0));

    QPainter painter(&outputData._atlasImage);
    for(auto itor = outputContent.begin(); itor != outputContent.end(); itor++ ) {
        if (_aborted) return false;

        const PolyPack2D::Content<PackContent> &content = *itor;

        // retreive your data.
        const PackContent &packContent = content.content();
        SpriteFrameInfo spriteFrame;

        spriteFrame.triangles = packContent.triangles();
        spriteFrame.frame = QRect(QPoint(content.bounds().left + _textureBorder, content.bounds().top + _textureBorder), QPoint(content.bounds().right, content.bounds().bottom));
        spriteFrame.offset = QPoint(
                    packContent.rect().left(),
                    packContent.rect().top()
                    );
        spriteFrame.rotated = false;
        spriteFrame.sourceColorRect = packContent.rect();
        spriteFrame.sourceSize = packContent.image().size();

        QPainterPath clipPath;
        for (auto polygon: packContent.polygons()) {
            clipPath.addPolygon(QPolygonF(QVector<QPointF>::fromStdVector(polygon)));
        }
        clipPath.translate(content.bounds().left + _textureBorder, content.bounds().top + _textureBorder);
        painter.setClipPath(clipPath);
        painter.drawImage(QPoint(content.bounds().left + _textureBorder, content.bounds().top + _textureBorder), packContent.image(), packContent.rect());

        outputData._spriteFrames[packContent.name()] = spriteFrame;

        // add ident to sprite frames
        auto identicalIt = _identicalFrames.find(packContent.name());
        if (identicalIt != _identicalFrames.end()) {
            QStringList identicalList;
            for (auto ident: (*identicalIt)) {
                outputData._spriteFrames[ident] = spriteFrame;

                identicalList.push_back(ident);
            }
        }
    }

    painter.end();
    _outputData.push_front(outputData);

    return true;
}

void SpriteAtlas::onPlaceCallback(int current, int count) {
    if (_progress)
        _progress->setProgressText(QString("Placing: %1/%2").arg(current).arg(count));
}
