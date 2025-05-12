#-------------------------------------------------
#
# Project created by QtCreator 2014-01-04T16:27:31
#
#-------------------------------------------------

QT += core widgets xml qml concurrent network

TARGET = SpriteSheetPacker
TEMPLATE = app

OBJECTS_DIR = obj
MOC_DIR = qt_gen
UI_DIR = qt_gen

CONFIG += c++11
#QMAKE_CXXFLAGS +=-std=c++11 -stdlib=libc++

CONFIG(release,debug|release) {
    win32: DESTDIR = $$PWD/../install/win/bin
    macx: DESTDIR = $$PWD/../install/macos/bin
    linux: DESTDIR = $$PWD/../install/linux/bin
} else {
    macx: DESTDIR = $$OUT_PWD
    win32: DESTDIR = $$OUT_PWD/debug
    linux: DESTDIR = $$OUT_PWD
}

INCLUDEPATH += 3rdparty

SOURCES += main.cpp \
    gui/AboutDialog.cpp \
    gui/AnimationDialog.cpp \
    gui/ContentProtectionDialog.cpp \
    gui/MainWindow.cpp \
    gui/PreferencesDialog.cpp \
    gui/PublishStatusDialog.cpp \
    gui/ScalingVariantWidget.cpp \
    gui/SpriteAtlasPreview.cpp \
    gui/SpritesTreeWidget.cpp \
    gui/StatusBarWidget.cpp \
    gui/UpdaterDialog.cpp \
    gui/ZoomGraphicsView.cpp \
    ImageFillOuter.cpp \
    PolygonImage2.cpp \
    SpriteAtlas.cpp \
    SpritePackerProjectFile.cpp \
    PngOptimizer.cpp \
    PublishSpriteSheet.cpp \
    command-line.cpp \
    ElapsedTimer.cpp \
    pack_content.cpp \
    triangles.cpp

HEADERS += \
    gui/AboutDialog.h \
    gui/AnimationDialog.h \
    gui/ContentProtectionDialog.h \
    gui/MainWindow.h \
    gui/PreferencesDialog.h \
    gui/PublishStatusDialog.h \
    gui/ScalingVariantWidget.h \
    gui/SpriteAtlasPreview.h \
    gui/SpritesTreeWidget.h \
    gui/StatusBarWidget.h \
    gui/UpdaterDialog.h \
    gui/ZoomGraphicsView.h \
    ImageFillOuter.h \
    ImageRotate.h \
    PolygonImage2.h \
    SpriteAtlas.h \
    SpritePackerProjectFile.h \
    PngOptimizer.h \
    PublishSpriteSheet.h \
    ImageFormat.h \
    ElapsedTimer.h \
    pack_content.h \
    triangles.h

#algorithm
INCLUDEPATH += algorithm

HEADERS += \
    algorithm/binpack2d.hpp \
    algorithm/polygon_pack_balmer.h

SOURCES += \
    algorithm/polygon_pack_balmer.cpp



#other...

FORMS += \
    gui/AboutDialog.ui \
    gui/AnimationDialog.ui \
    gui/ContentProtectionDialog.ui \
    gui/MainWindow.ui \
    gui/PreferencesDialog.ui \
    gui/PublishStatusDialog.ui \
    gui/ScalingVariantWidget.ui \
    gui/SpriteAtlasPreview.ui \
    gui/StatusBarWidget.ui \
    gui/UpdaterDialog.ui

RESOURCES += resources.qrc

include(TPSParser/TPSParser.pri)
include(3rdparty/optipng/optipng.pri)
include(3rdparty/qtplist-master/qtplist-master.pri)
include(3rdparty/clipper/clipper.pri)
include(3rdparty/poly2tri/poly2tri.pri)
include(3rdparty/pngquant/pngquant.pri)
include(3rdparty/lodepng/lodepng.pri)
include(3rdparty/PVRTexTool/PVRTexTool.pri)
include(../tests/PolygonImage/poly/balmer_poly.pri)

OTHER_FILES += \
    defaultFormats/cocos2d.js \
    defaultFormats/cocos2d-old.js \
    defaultFormats/pixijs.js \
    defaultFormats/phaser.js \
    defaultFormats/json.js

macx {
    ICON = SpritePacker.icns
    exportFormats.files = $$files(defaultFormats/*.*)
    exportFormats.path = Contents/MacOS/defaultFormats
    QMAKE_BUNDLE_DATA += exportFormats
}

win32 {
    CONFIG += console
    RC_ICONS = SpritePacker.ico

    QMAKE_PRE_LINK += if not exist $$shell_quote($$shell_path($$DESTDIR/defaultFormats)) mkdir $$shell_quote($$shell_path($$DESTDIR/defaultFormats)) $$escape_expand(\\n\\t)
    FILES = $$files(defaultFormats/*.*)
    for(FILE, FILES) {
        QMAKE_PRE_LINK += $${QMAKE_COPY} $$shell_quote($$shell_path($$PWD/$$FILE)) $$shell_quote($$shell_path($$DESTDIR/$$FILE)) $$escape_expand(\\n\\t)
    }
}

CONFIG(release,debug|release) {
    # release
    win32 {
        #QMAKE_POST_LINK = windeployqt $$shell_quote($$shell_path($${DESTDIR}/$${TARGET}.exe))
    }

    macx {
        QMAKE_POST_LINK = macdeployqt $$shell_quote($$shell_path($${DESTDIR}/$${TARGET}.app))
    }
}
