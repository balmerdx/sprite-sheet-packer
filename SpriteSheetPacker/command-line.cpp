#include <QtCore>
#include "SpriteAtlas.h"
#include "PublishSpriteSheet.h"
#include "SpritePackerProjectFile.h"
bool verbose = false;

int commandLine(QCoreApplication& app) {
    QCommandLineParser parser;
    parser.setApplicationDescription("");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("source", "Sprites for packing or project file (You can override project file options with [options]).");
    parser.addPositionalArgument("destination", "Destination folder where you're saving the sprite sheet. Optional when using project file");

    //!!!!!! Некоторые опции не работают из командной строки, только из файла проекта !!!!!!!
    parser.addOptions({
        {{"f", "format"}, "Format for export sprite sheet data. Default is cocos2d.", "format"},
        {"trimMode", "Rect - Removes the transparency around a sprite. The sprites appear to have their original size when using them.\n"
         "Polygon - The amount of rendered transparency can be reduced by creating a tight fitting polygon around the solid pixels of a sprite. But: The vertices must be transformed by the CPU — introducing new costs.\n"
         "Default is Rect", "mode", "Rect"},
        {"algorithm", "Rect or Polygon. Default is Rect", "mode", "Rect"},
        {"trim", "Allowed values: 1 to 255, default is 1. Pixels with an alpha value below this value will be considered transparent when trimming the sprite. Very useful for sprites with nearly invisible alpha pixels at the borders.", "int", "1"},
        {"epsilon", "Lower values create a tighter fitting mesh with less transparency but with more vertices.\nHigher values on the other hand reduce the number of vertices at the cost of adding more transparency.", "float", "5"},
        {"texture-border", "Border of the sprite sheet, value adds transparent pixels around the borders of the sprite sheet. Default value is 0.", "int", "0"},
        {"sprite-border", "Sprite border is the space between sprites. Value adds transparent pixels between sprites to avoid artifacts from neighbor sprites. The transparent pixels are not added to the sprites, default is 2.", "int", "2"},
        {"powerOf2", "Forces the texture to have power of 2 size (32, 64, 128...). Default is disable."},
        {"max-size", "Sets the maximum size for the texture, default is 8192.", "size", "8192"},
        {"png-opt-mode", "Optimizes the png's file size.\n"
         "None - No optimization at all(fastest).\n"
         "Lossless - Uses optipng to optimize the filesize. The reduction is mostly small but doesn't harm image quality.\n"
         "Lossy - Uses pngquant to optimize the filesize. The reduction is mostly about 70%, but the image quality gets a bit worse.", "int", "0"},
        {"png-opt-level", "Optimizes the image's file size. Only useful in combination with opt-mode Lossless. Allowed values: 1 to 7 (Using a high value might take some time to optimize.", "int", "0"},
        {"scale", "Scales all images before creating the sheet. E.g. use 0.5 for half size, default is 1 (Scale has no effect when source is a project file).", "float", "1"},
        {"trimSpriteNames", "Remove image file extensions from the sprite names - e.g. .png, .jpg, ...", "bool", "false"},
        {"prependSmartFolderName", "Prepends the smart folder's name as part of the sprite name.", "bool", "false"},
        {"rotateSprites", "Enable rotate sprites. (Only in project)", "bool", "false"},
        {"error-if-not-fit", "Generate error if not fit in one texture (default 0)", "bool", "0"},
        {"verbose", "Verbose output (default 0)", "bool", "0"},
        {"enable-find-identical", "Enable find identical sprite (default 1)", "bool", "1"},
        {"granularityX", "Set granularity x (default 1)", "int", "1"},
        {"granularityY", "Set granularity y (default 1)", "int", "1"},
        {"source-list-file", "Override default image source list. The paths in the file are relative to the project file directory (or shuld be absolute path).", "path", ""},
        {"make-polygon-info", "Set directory to store polygon info.", "path", ""},
        {"use-polygon-info", "Set directory to read polygon info.", "path", ""},
        {"disable-save-image", "Then publish save dont save images"},
        {"trim-rect-list", "Trim this sprites as rect instead poygon.", "path", ""}
    });

    parser.process(app);

    bool destinationSet = true;
    SpritePackerProjectFile* projectFile = nullptr;

    if (parser.positionalArguments().size() > 2) {
        qDebug() << "Too many arguments, see help for information.";
        parser.showHelp();
        return -1;
    } else if ((parser.positionalArguments().size() == 1) || (parser.positionalArguments().size() == 2)) {
        QFileInfo src(parser.positionalArguments().at(0));
        if(!src.exists())
        {
            qDebug() << "File not found.";
            qDebug() << "Filename:" << parser.positionalArguments().at(0);
            return -1;
        }

        if (!src.isDir()) {
            std::string suffix = src.suffix().toStdString();
            projectFile = SpritePackerProjectFile::factory().get(suffix)();
        }

        if (parser.positionalArguments().size() == 1) {
            if (!projectFile) {
                qDebug() << "Arguments must have source and destination, see help for information.";
                parser.showHelp();
                return -1;
            } else {
                destinationSet = false; // we should already have our destination saved in our project file
            }
        } else if (parser.positionalArguments().size() == 2) {
            destinationSet = true;
        }

    } else {
        qDebug() << "Arguments must have source and destination, see help for information.";
        parser.showHelp();
        return -1;
    }

    if(verbose)
        qDebug() << "arguments:" << parser.positionalArguments() << "options:" << parser.optionNames();

    QFileInfo source(parser.positionalArguments().at(0));
    QDir destination;

    if (destinationSet) {
        destination = QDir(parser.positionalArguments().at(1));
    }

    // initialize [options]
    QString trimMode = "Rect";
    QString algorithm = "Rect";
    int trim = 1;
    float epsilon = 5.f;
    int textureBorder = 0;
    int spriteBorder = 2;
    bool pow2 = false;
    bool forceSquared = false;
    bool heuristicMask = false;
    QSize maxSize(8192, 8192);
    float imageScale = 1;
    QString format = "cocos2d";
    QString pngOptMode = "None";
    int pngOptLevel = 0;
    bool trimSpriteNames = false;
    bool prependSmartFolderName = false;
    bool rotateSprites = false;
    bool enableFindIdentical = true;
    bool errorIfNotFit = false;
    QSize granularity(1, 1);
    bool enableMakePolygonInfo = false;
    bool enableUsePolygonInfo = false;
    QDir storePolygonInfoDir;
    bool enableOverrideSourceListFile = false;
    QStringList overrideSourceListFile;
    bool enableSaveImage = true;
    QString trimRectList;

    if (projectFile) {
        if (!projectFile->read(source.filePath())) {
            qCritical() << "File format error.";
            return -1;
        } else {
            trimMode = projectFile->trimMode();
            algorithm = projectFile->algorithm();
            trim = projectFile->trimThreshold();
            epsilon = projectFile->epsilon();
            textureBorder = projectFile->textureBorder();
            spriteBorder = projectFile->spriteBorder();
            pngOptMode = projectFile->pngOptMode();
            pngOptLevel = projectFile->pngOptLevel();
            trimSpriteNames = projectFile->trimSpriteNames();
            prependSmartFolderName = projectFile->prependSmartFolderName();
            rotateSprites = projectFile->rotateSprites();
            granularity = projectFile->granularity();
            enableFindIdentical = projectFile->enableFindIdentical();

            if (!destinationSet) {
                destination = QDir(projectFile->destPath());
            }
        }
    }

    if (!destination.exists()) {
        if(!destination.mkpath(destination.path()))
        {
            qDebug() << "Incorrect destination folder" << destination;
            return -1;
        }
    }

    // you can override project file options
    if (parser.isSet("trimMode")) {
        trimMode = parser.value("trimMode");
    }
    if (parser.isSet("algorithm")) {
        algorithm = parser.value("algorithm");
    }
    if (parser.isSet("trim")) {
        trim = parser.value("trim").toInt();
    }
    if (parser.isSet("epsilon")) {
        epsilon = parser.value("epsilon").toFloat();
    }
    if (parser.isSet("texture-border")) {
        textureBorder = parser.value("texture-border").toInt();
    }
    if (parser.isSet("sprite-border")) {
        spriteBorder = parser.value("sprite-border").toInt();
    }
    if (parser.isSet("powerOf2")) {
        pow2 = true;
    }
    if (parser.isSet("max-size")) {
        int sz = parser.value("max-size").toInt();
        maxSize = QSize(sz, sz);
    }
    if (parser.isSet("scale") && !projectFile) {
        imageScale = parser.value("scale").toFloat();
    }
    if (parser.isSet("format")) {
        format = parser.value("format");
    }

    if (parser.isSet("png-opt-mode")) {
         pngOptMode = parser.value("png-opt-mode");
    }

    if (parser.isSet("png-opt-level")) {
        pngOptLevel = parser.value("png-opt-level").toInt();
        pngOptLevel = qBound(1, pngOptLevel, 7);
    }

    if (parser.isSet("enable-find-identical")) {
        enableFindIdentical = parser.value("enable-find-identical").toInt() ? true : false;
    }

    if (parser.isSet("error-if-not-fit")) {
        errorIfNotFit = parser.value("error-if-not-fit").toInt() ? true : false;
    }

    if (parser.isSet("verbose")) {
        verbose = parser.value("verbose").toInt() ? true : false;
    }


    if (parser.isSet("granularityX")) {
        granularity.setWidth(parser.value("granularityX").toInt());
    }

    if (parser.isSet("granularityY")) {
        granularity.setHeight(parser.value("granularityY").toInt());
    }

    if (granularity.width() != 1 || granularity.height() != 1)
    {
/*
        if (textureBorder != 0 )
        {
            qDebug() << "Granularity in settings require textureBorder=0";
            return -1;
        }

        if (trim != 0 )
        {
            qDebug() << "Granularity in settings require trim=0";
            return -1;
        }
*/
        if (rotateSprites)
        {
            qDebug() << "Granularity in settings require rotateSprites=false";
            return -1;
        }
    }

    if (parser.isSet("make-polygon-info"))
    {
        auto value = parser.value("make-polygon-info");
        if(verbose) qDebug() << "make-polygon-info:" << value;
        enableMakePolygonInfo = true;
        storePolygonInfoDir = QDir(value);
        if(!storePolygonInfoDir.mkpath("."))
        {
            qCritical() << "Cannot create path :" << value;
            return -1;
        }
    }

    if (parser.isSet("use-polygon-info"))
    {
        auto value = parser.value("use-polygon-info");
        if(verbose) qDebug() << "use-polygon-info:" << value;
        enableUsePolygonInfo = true;
        storePolygonInfoDir = QDir(value);
    }

    if (parser.isSet("source-list-file"))
    {
        auto listFilename = parser.value("source-list-file");
        if(verbose) qDebug() << "source-list-file :" << listFilename;

        enableOverrideSourceListFile = true;
        if(!SpritePackerProjectFile::loadFilesList(source.absoluteDir(), listFilename, "source-list-file", overrideSourceListFile, !enableUsePolygonInfo))
        {
            return -1;
        }

        //if(verbose) qDebug() << "overrideSourceListFile :" << overrideSourceListFile;
        //if(verbose) qDebug() << "projectFile->srcList() :" << projectFile->srcList();
    }

    if (parser.isSet("disable-save-image"))
    {
        enableSaveImage = false;
    }

    if (parser.isSet("trim-rect-list"))
    {
        trimRectList = parser.value("trim-rect-list");
        if(verbose) qDebug() << "trim-rect-list :" << trimRectList;
    }

    if(verbose)
    {
        qDebug() << "trimMode:" << trimMode;
        qDebug() << "algorithm:" << algorithm;
        qDebug() << "trim:" << trim;
        qDebug() << "epsilon:" << epsilon;
        qDebug() << "textureBorder:" << textureBorder;
        qDebug() << "spriteBorder:" << spriteBorder;
        qDebug() << "pow2:" << pow2;
        qDebug() << "maxSize:" << maxSize;
        qDebug() << "scale:" << imageScale;
        qDebug() << "png-opt-mode:" << pngOptMode;
        qDebug() << "png-opt-level:" << pngOptLevel;
        qDebug() << "rotateSprites:" << rotateSprites;
        qDebug() << "destination:" << destination.absolutePath();
        qDebug() << "granularity:" << granularity;
    }

    // load formats
    QSettings settings;
    QStringList formatsFolder;
    formatsFolder.push_back(QCoreApplication::applicationDirPath() + "/defaultFormats");

    {
        auto custom = settings.value("Preferences/customFormatFolder").toString();
        if (!custom.isEmpty())
            formatsFolder.push_back(custom);
    }

    PublishSpriteSheet publisher;

    if(projectFile)
    {
        publisher.setPremultiplied(projectFile->premultiplied());
    }
    publisher.setEnableSaveImage(enableSaveImage);

    if (projectFile && !trimRectList.isEmpty())
    {
        QStringList trimRectListFiles;
        if(!SpritePackerProjectFile::loadFilesList(source.absoluteDir(), trimRectList, "trimRectList", trimRectListFiles, !enableUsePolygonInfo))
        {
            qCritical() << "Cannot load:" << trimRectList;
            return -1;
        }

        projectFile->setTrimRectList(trimRectListFiles);
    }


    // load formats
    PublishSpriteSheet::formats().clear();
    for (auto folder: formatsFolder) {
        if (QDir(folder).exists()) {
            QDirIterator fileNames(folder, QStringList() << "*.js", QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot);
            while(fileNames.hasNext()) {
                fileNames.next();
                PublishSpriteSheet::addFormat(fileNames.fileInfo().baseName(), fileNames.filePath());
            }
        }
    }
    if(verbose)
        qDebug() << "Support Formats:" << PublishSpriteSheet::formats().keys();

    if (projectFile) {
        for (int i=0; i<projectFile->scalingVariants().size(); ++i) {
            ScalingVariant variant = projectFile->scalingVariants().at(i);

            QString variantName = variant.name;
            float scale = variant.scale;
            maxSize = variant.maxTextureSize;
            pow2 = variant.pow2;
            QString spriteSheetName = projectFile->spriteSheetName();
            if (spriteSheetName.contains("{v}")) {
                spriteSheetName.replace("{v}", variantName);
            } else {
                spriteSheetName = variantName + spriteSheetName;
            }
            while (spriteSheetName.at(0) == '/') {
                spriteSheetName.remove(0, 1);
            }

            QFileInfo destFileInfo(destination.absoluteFilePath(spriteSheetName));

            {
                QDir dir = destFileInfo.dir();
                dir.mkpath(".");
            }

            // Generate sprite atlas
            SpriteAtlas atlas(QStringList() << (enableOverrideSourceListFile ? overrideSourceListFile : projectFile->srcList()),
                              projectFile->getTrimRectList(),
                              textureBorder, spriteBorder, trim, heuristicMask, pow2,
                              forceSquared, maxSize, scale, granularity, variant.fixedTextureSize);
            atlas.setRotateSprites(rotateSprites, projectFile->rotateSpritesCw());
            atlas.enableFindIdentical(enableFindIdentical);
            atlas.setProjectDir(source.absoluteDir());

            if (enableMakePolygonInfo)
            {
                atlas.enableFindIdentical(false);
                atlas.setStorePolygonInfoDir(storePolygonInfoDir);
            }

            if (enableUsePolygonInfo)
            {
                atlas.setUsePolygonInfoDir(storePolygonInfoDir);
            }

            if (trimMode == "Polygon") {
                atlas.enablePolygonMode(true, epsilon);
            }
            if (algorithm == "Polygon") {
                atlas.setAlgorithm(algorithm);
            }
            if (!atlas.generate()) {
                qCritical() << "ERROR: Generate atlas! :"  << source.filePath();
                return -1;
            }

            if (enableMakePolygonInfo)
            {
                return 0;
            }

            publisher.addSpriteSheet(atlas, destFileInfo.filePath());

            if (!parser.isSet("format")) {
                format = projectFile->dataFormat();
            }

        }

        delete projectFile;
        projectFile = nullptr;
    } else {
        // Generate sprite atlas
        SpriteAtlas atlas(QStringList() << source.filePath(), QStringList(), textureBorder, spriteBorder, trim, heuristicMask, pow2,
                          forceSquared, maxSize, imageScale, granularity);
        //!!Возможно вращение в неверную сторону!!
        atlas.setRotateSprites(rotateSprites, true);
        atlas.enableFindIdentical(enableFindIdentical);
        if (trimMode == "Polygon") {
            atlas.enablePolygonMode(true, epsilon);
        }
        if (algorithm == "Polygon") {
         atlas.setAlgorithm(algorithm);
        }
        if (!atlas.generate()) {
            qCritical() << "ERROR: Generate atlas!";
            return -1;
        }

        publisher.addSpriteSheet(atlas, destination.absoluteFilePath(source.fileName()));
    }

    if(errorIfNotFit && publisher.notFitInOneTexture())
    {
        qCritical() << "ERROR: Multiple textures not available! :" << source.filePath();
        return -1;
    }

    publisher.setTrimSpriteNames(trimSpriteNames);
    publisher.setPrependSmartFolderName(prependSmartFolderName);
    publisher.setPngQuality(pngOptMode, pngOptLevel);

    if (!publisher.publish(format, false)) {
        qCritical() << "ERROR: publish atlas!" << source.filePath();
        return -1;
    }

    if(verbose)
        qDebug() << "Publishing is finished.";

    return 0;
}
