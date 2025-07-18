#include "pack_content.h"

PackContent::PackContent() {
    // only for QVector
    //qDebug() << "PackContent::PackContent()";
}
PackContent::PackContent(const QString& name, const QImage& image) {
    _name = name;
    //auto format = image.format();
    if(image.format() != QImage::Format_ARGB32)
        _image = image.convertToFormat(QImage::Format_ARGB32);
    else
        _image = image;
    _rect = QRect(0, 0, _image.width(), _image.height());
}

bool PackContent::isIdentical(const PackContent& other) {
    if (_rect != other._rect) return false;

    for (int x = _rect.left(); x <= _rect.right(); ++x) {
        for (int y = _rect.top(); y <= _rect.bottom(); ++y) {
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
