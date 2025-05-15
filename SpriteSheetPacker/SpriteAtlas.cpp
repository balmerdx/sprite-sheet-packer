#include "SpriteAtlas.h"

#include <functional>
#include "ImageRotate.h"
#include "binpack2d.hpp"
#include "PolygonImage2.h"
#include "ImageFillOuter.h"
#include "ElapsedTimer.h"
#include "polygon_pack_balmer.h"

int pow2(int len) {
    int order = 1;
    while(pow(2,order) < len)
    {
       order++;
    }
    return pow(2,order);
}

SpriteAtlas::SpriteAtlas(const QStringList& sourceList, const QStringList &trimRectListFiles, int textureBorder, int spriteBorder, int trim, bool heuristicMask, bool pow2, bool forceSquared,
                         QSize maxSize, float scale, QSize granularity, QSize fixedTextureSize)
    : _sourceList(sourceList)
    , _trim(trim)
    , _textureBorder(textureBorder)
    , _spriteBorder(spriteBorder)
    , _heuristicMask(heuristicMask)
    , _pow2(pow2)
    , _forceSquared(forceSquared)
    , _maxTextureSize(maxSize)
    , _fixedTextureSize(fixedTextureSize)
    , _scale(scale)
    , _granularity(granularity)
{
    _polygonMode.enable = false;
    _trimRectListFiles = QSet<QString>(trimRectListFiles.begin(), trimRectListFiles.end());
}

void SpriteAtlas::enablePolygonMode(bool enable, float epsilon) {
    _polygonMode.enable = enable;
    _polygonMode.epsilon = epsilon;
}

bool SpriteAtlas::generate(SpriteAtlasGenerateProgress* progress) {
    _aborted = false;

    ElapsedTimer timePerform(nullptr);
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
            if (_isValidProjectDir)
            {
                QString relativePath = _projectDir.relativeFilePath(pathName);
                fileList.push_back(qMakePair(pathName, relativePath));
            } else
            {
                fileList.push_back(qMakePair(pathName, fi.fileName()));
            }
        }
    }

    int skipSprites = 0;
    // init images and rects
    _identicalFrames.clear();

    int progressIndex = 1;
    QVector<PackContent> inputContent;

    auto addIdentical = [this, &skipSprites, &inputContent](PackContent& packContent) -> bool
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

        return findIdentical;
    };

    if (_enableUsePolygonInfo)
    {
        for(auto it_f = fileList.begin(); it_f != fileList.end(); ++it_f, ++progressIndex) {
            if (_aborted) return false;
            PackContent packContent;
            if (!packContent.load(_storePolygonInfoDir, it_f->second))
                return false;

            if(_enableFindIdentical)
            {
                if (addIdentical(packContent))
                    continue;
            }

            inputContent.push_back(packContent);
        }
    } else
    {
        for(auto it_f = fileList.begin(); it_f != fileList.end(); ++it_f, ++progressIndex) {
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
                if (_polygonMode.enable) {
                    bool packToRect = _trimRectListFiles.contains((*it_f).first);
                    if (verbose)
                        qDebug() << (*it_f).first << "packToRect ="<<packToRect;
                    PolygonImage2 polygonImage(packContent.image(), packContent.rect(), packToRect);
                    packContent.setTriangles(polygonImage.triangles());
                    /* test code
                    QImage tmp_img = packContent.triangles().drawTriangles();
                    tmp_img.save("tmp_img.png");
                    exit(1);
                    */
                } else
                {
                    packContent.trim(_trim);
                }
            }

            if(_enableFindIdentical)
            {
                if (addIdentical(packContent))
                    continue;
            }

            inputContent.push_back(packContent);
        }
    }

    if (skipSprites)
        qDebug() << "Total skip sprites: " << skipSprites;

    bool result = false;
    if ((_algorithm == "Polygon") && (_polygonMode.enable)) {
        result = packWithPolygon(inputContent);
    } else {
        result = packWithRect(inputContent);
    }

    //if(verbose)
        qDebug() << "Generate time:" <<  QString::number(timePerform.elapsed_sec(), 'f', 3) << "sec";

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
        int width = GranularityDivSizeX(packContent.rect().width() + _spriteBorder);
        int height = GranularityDivSizeY(packContent.rect().height() + _spriteBorder);
        volume += width * height * 1.02f;

        inputContent += BinPack2D::Content<PackContent>(packContent,
                                                        BinPack2D::Coord(),
                                                        BinPack2D::Size(width, height),
                                                        _rotateSprites,
                                                        false);
    }

    // Sort the input content by size... usually packs better.
    inputContent.Sort();

    // A place to store packed content.
    BinPack2D::ContentAccumulator<PackContent> remainder;
    BinPack2D::ContentAccumulator<PackContent> outputContent;

    auto makeTexSizeG = [this](int w, int h) -> QSize
    {
        return QSize(GranularityDivSizeX(w - _textureBorder*2), GranularityDivSizeY(h - _textureBorder*2));
    };

    // find optimal size for atlas
    int w = qMin(_maxTextureSize.width(), (int)sqrt(volume));
    int h = qMin(_maxTextureSize.height(), (int)sqrt(volume));

    if (_fixedTextureSize.width() > 0)
        w = _fixedTextureSize.width();
    if (_fixedTextureSize.height() > 0)
        h = _fixedTextureSize.height();

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

            auto texSizeG = makeTexSizeG(w, h);
            BinPack2D::CanvasArray<PackContent> canvasArray = BinPack2D::UniformCanvasArrayBuilder<PackContent>(
                                                                  texSizeG.width(), texSizeG.height(), 1).Build();

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
            auto texSizeG = makeTexSizeG(w, h);
            BinPack2D::CanvasArray<PackContent> canvasArray = BinPack2D::UniformCanvasArrayBuilder<PackContent>(
                                                                  texSizeG.width(), texSizeG.height(), 1).Build();

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
                auto texSizeG = makeTexSizeG(w, h);
                BinPack2D::CanvasArray<PackContent> canvasArray = BinPack2D::UniformCanvasArrayBuilder<PackContent>(
                                                                      texSizeG.width(), texSizeG.height(), 1).Build();

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

            auto texSizeG = makeTexSizeG(w, h);
            BinPack2D::CanvasArray<PackContent> canvasArray = BinPack2D::UniformCanvasArrayBuilder<PackContent>(
                                                                  texSizeG.width(), texSizeG.height(), 1).Build();

            bool success = canvasArray.Place(inputContent, remainder);
            if (success) {
                outputContent = BinPack2D::ContentAccumulator<PackContent>();
                canvasArray.CollectContent(outputContent);
                break;
            } else {
                if ((w == _maxTextureSize.width()) && (h == _maxTextureSize.height())) {
                    qDebug() << "Max size Limit!";

                    if(tryPermutation(inputContent, outputContent, texSizeG.width(), texSizeG.height(),
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
        if (_fixedTextureSize.width() <= 0)
        while (w) {
            if (_aborted) return false;

            w -= step;
            if (_forceSquared) {
                h = w;
            }
            auto texSizeG = makeTexSizeG(w, h);
            BinPack2D::CanvasArray<PackContent> canvasArray = BinPack2D::UniformCanvasArrayBuilder<PackContent>(
                                                                  texSizeG.width(), texSizeG.height(), 1).Build();

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

        if (_fixedTextureSize.height() <= 0)
        if (!_forceSquared) {
            step = qMax((w + h) / 20, 1);
            while (h) {
                if (_aborted) return false;

                h -= step;
                auto texSizeG = makeTexSizeG(w, h);
                BinPack2D::CanvasArray<PackContent> canvasArray = BinPack2D::UniformCanvasArrayBuilder<PackContent>(
                                                                      texSizeG.width(), texSizeG.height(), 1).Build();

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

    w = GranularityRoundUpX(w);
    h = GranularityRoundUpY(h);

    std::vector<QRect> paintedRects;

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
            if(_rotateSpritesCw)
                image = rotate90(image);
            else
                image = rotate270(image);
        }

        SpriteFrameInfo spriteFrame;
        spriteFrame.triangles = packContent.triangles();
        spriteFrame.frame = QRect(GranularityMulPosX(content.coord.x + _textureBorder),
                                  GranularityMulPosY(content.coord.y + _textureBorder),
                                  packContent.rect().width(), packContent.rect().height());
        if (spriteFrame.triangles.indices.size()) {
            spriteFrame.offset = QPoint(
                        packContent.rect().left(),
                        packContent.rect().top()
                        );
        } else {
            QSize imageSize = QSize(packContent.image().width(), packContent.image().height());
            auto content_w = packContent.rect().width();
            auto content_h = packContent.rect().height();

            if(content.rotated)
            {
                //По хорошему _rotateSpritesCw надо заменить на тип формата, но пока пускай так будет
                if(_rotateSpritesCw)
                {
                    //cocos2d format
                    spriteFrame.offset = QPoint(
                                (packContent.rect().left() + (-imageSize.width() + content_w) * 0.5f),
                                (-packContent.rect().top() + ( imageSize.height() - content_h) * 0.5f)
                                );
                } else
                {
                    //libGDX / spine-atlas format
                    spriteFrame.offset = QPoint(
                                (packContent.rect().left() + (-imageSize.width() + content_w)),
                                (-packContent.rect().top() + ( imageSize.height() - content_h))
                                );
                }
            } else
            {
                if(_rotateSpritesCw)
                {
                    spriteFrame.offset = QPoint(
                                (packContent.rect().left() + (-imageSize.width() + content_w) * 0.5f),
                                (-packContent.rect().top() + ( imageSize.height() - content_h) * 0.5f)
                                );
                } else
                {
                    spriteFrame.offset = QPoint(
                                (packContent.rect().left() + (-imageSize.width() + content_w)),
                                (-packContent.rect().top() + ( imageSize.height() - content_h))
                                );
                }
            }
        }
        spriteFrame.rotated = content.rotated;
        spriteFrame.sourceColorRect = packContent.rect();
        spriteFrame.sourceSize = packContent.image().size();
        if (content.rotated) {
            spriteFrame.frame = QRect(GranularityMulPosX(content.coord.x),
                                      GranularityMulPosY(content.coord.y),
                                      packContent.rect().height(), packContent.rect().width());
        }

        QPoint imagePos = spriteFrame.frame.topLeft();
        QSize imageSize;
        if (content.rotated) {
            imageSize = image.size();
            painter.drawImage(imagePos, image);
        } else {
            imageSize = packContent.rect().size();
            painter.drawImage(imagePos, packContent.image(), packContent.rect());
        }

        paintedRects.push_back(QRect(imagePos, imageSize));

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

    {
        /*
           Чтобы не ломать старое поведение, оставляем как было.
           _spriteBorder = 1 - это бордюр только справа-снизу (и он не заполняется).
           Если требуется бордюр со всех сторон шириной в 1 пиксель, то надо выставить _spriteBorder = 2.
           Предполагаем, что у текстур выставлен clamp, поэтому по границе слева-сверху бордюра не будет, и это нормально.
        */
        int border = _spriteBorder / 2;
        if(border > 0)
        for(QRect sprite_size : paintedRects)
        {
            fillOuter(outputData._atlasImage, sprite_size, border);
        }
    }


    _outputData.push_front(outputData);

    return true;
}

bool SpriteAtlas::packWithPolygon(const QVector<PackContent>& content) {
    if (_progress)
        _progress->setProgressText("Build pack contents...");

    std::vector<PackContent> contents_std;
    contents_std.insert(contents_std.end(), content.begin(), content.end());
    PolygonPackBalmer polygon_pack(verbose);

    polygon_pack.setMaxTextureSize(_maxTextureSize);
    polygon_pack.setGranularity(_granularity);
    polygon_pack.setSpriteBorder(_spriteBorder);
    polygon_pack.setContent(contents_std);

    if (_enableStorePolygonInfo)
    {
        if (!_isValidProjectDir)
        {
            qCritical() << "_isValidProjectDir==false";
            return false;
        }

        //Не пакуем, а только сохраняем полигональную информацию о наших изображениях
        auto outputContent = polygon_pack.contentList();
        for(auto& content : outputContent) {
            content.save(_storePolygonInfoDir);
        }
        return true;
    }

    polygon_pack.place();

    auto outputContent = polygon_pack.contentList();

    OutputData outputData;

    outputData._atlasImage = QImage(polygon_pack.bounds().width(), polygon_pack.bounds().height(), QImage::Format_RGBA8888);
    outputData._atlasImage.fill(QColor(0, 0, 0, 0));

    //Копируем попиксельно, только достаточно непрозрачные пиксели (>_trim)
    auto copy_rect = [](QImage& dst, const QImage& src, QPoint pos, QRect src_rect, int trim_alpha)
    {
        int xmin = src_rect.left();
        int ymin = src_rect.top();
        int xmax = src_rect.right();
        int ymax = src_rect.bottom();

        xmin = std::max(0, xmin);
        xmax = std::min(src.width(), xmax);
        ymin = std::max(0, ymin);
        ymax = std::min(src.height(), ymax);

        int xoffset = pos.x() - src_rect.left();
        int yoffset = pos.y() - src_rect.top();

        for(int y=ymin; y<ymax; y++)
        {
            for(int x=xmin; x<xmax; x++)
            {
                QRgb rgb = src.pixel(x, y);
                if (qAlpha(rgb) > trim_alpha)
                    dst.setPixel(x + xoffset, y + yoffset, rgb);
            }
        }
    };

    auto copy_border = [](QImage& dst, const AImage& src, QPoint pos)
    {
        int xoffset = pos.x();
        int yoffset = pos.y();
        QRgb color = qRgb(255, 0, 255);

        for(int y=0; y<src.height(); y++)
        {
            for(int x=0; x<src.width(); x++)
            {
                if (src.get(x, y) && x + xoffset < dst.width() && y + yoffset < dst.height())
                    dst.setPixel(x + xoffset, y + yoffset, color);
            }
        }
    };


    for(auto& content : outputContent) {
        if (_aborted) return false;

        // retreive your data.
        const PackContent &packContent = content.content();
        SpriteFrameInfo spriteFrame;

        spriteFrame.triangles = packContent.triangles();
        spriteFrame.frame = content.bounds();
        spriteFrame.offset = content.initial_bound().topLeft();
        spriteFrame.rotated = false;
        spriteFrame.sourceColorRect = packContent.rect();
        spriteFrame.sourceSize = packContent.image().size();
        spriteFrame.polygon_mask = content.mask.toJson();

        copy_rect(outputData._atlasImage, packContent.image(),
                  content.bounds().topLeft(),
                  content.initial_bound(), _trim);

        //test code. Рисуем бордюр на атлас.
        //copy_border(outputData._atlasImage, content.pixel_border, content.bounds().topLeft());

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


    _outputData.push_front(outputData);

    return true;
}

void SpriteAtlas::onPlaceCallback(int current, int count) {
    if (_progress)
        _progress->setProgressText(QString("Placing: %1/%2").arg(current).arg(count));
}
