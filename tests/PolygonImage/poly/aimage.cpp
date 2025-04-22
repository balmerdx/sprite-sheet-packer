#include "aimage.h"

#include <utility>
//#include <memory.h>
#include <algorithm>
#include <assert.h>

AImage::AImage()
{
}

AImage::~AImage()
{
    delete[] _data;
}

AImage::AImage(AImage&& src)
{
    *this = std::move(src);
}

void AImage::operator=(AImage&& d)
{
    _width = d._width;
    _height = d._height;
    _data = d._data;

    d._width = 0;
    d._height = 0;
    d._data = nullptr;
}

AImage::AImage(int width, int height, uint8_t fill)
{
    _width = width;
    _height = height;
    _data = new uint8_t[_width*_height];
    std::fill_n(_data, _width*_height, fill);
    //memset(_data, fill, _width*_height);
}

AImage::AImage(const QImage& img)
{
    _width = img.width();
    _height = img.height();
    _data = new uint8_t[_width*_height];

    for(int y=0; y<_height; y++)
    {
        for(int x=0; x<_width; x++)
        {
            _data[x+y*_width] = qAlpha(img.pixel(x,y));
        }
    }
}

uint8_t AImage::get(int x, int y) const
{
    bool in = this->in(x, y);
    assert(in);
    if (!in)
        return 0;

    return _data[x + y * _width];
}

uint8_t AImage::getb(int x, int y) const
{
    bool in = this->in(x, y);
    if (!in)
        return 0;

    return _data[x + y * _width];
}

void AImage::set(int x, int y, uint8_t value)
{
    bool in = this->in(x, y);
    assert(in);
    if (!in)
        return;

    _data[x + y * _width] = value;
}


QImage AImage::qalpha() const
{
    //QImage out(_width, _height, QImage::Format_Alpha8);
    QImage out(_width, _height, QImage::Format_ARGB32);

    for(int y=0; y<_height; y++)
    {
        for(int x=0; x<_width; x++)
        {
            out.setPixel(x, y, qRgba(255, 255, 255, _data[x+y*_width]));
        }
    }

    return std::move(out);
}

QImage AImage::qgray() const
{
    QImage out(_width, _height, QImage::Format_Grayscale8);
    //QImage out(_width, _height, QImage::Format_ARGB32);

    for(int y=0; y<_height; y++)
    {
        for(int x=0; x<_width; x++)
        {
            int v = _data[x+y*_width];
            out.setPixel(x, y, qRgb(v, v, v));
        }
    }

    return std::move(out);
}

/////////////////
AImage32::AImage32()
{
}

AImage32::~AImage32()
{
    delete[] _data;
}

AImage32::AImage32(AImage32&& src)
{
    *this = std::move(src);
}

void AImage32::operator=(AImage32&& d)
{
    _width = d._width;
    _height = d._height;
    _data = d._data;

    d._width = 0;
    d._height = 0;
    d._data = nullptr;
}

AImage32::AImage32(int width, int height, uint8_t fill)
{
    _width = width;
    _height = height;
    _data = new uint32_t[_width*_height];
    std::fill_n(_data, _width*_height, fill);
    //memset(_data, fill, _width*_height);
}

uint32_t AImage32::get(int x, int y) const
{
    bool in = this->in(x, y);
    assert(in);
    if (!in)
        return 0;

    return _data[x + y * _width];
}

uint32_t AImage32::getb(int x, int y) const
{
    bool in = this->in(x, y);
    if (!in)
        return 0;

    return _data[x + y * _width];
}

void AImage32::set(int x, int y, uint32_t value)
{
    bool in = this->in(x, y);
    assert(in);
    if (!in)
        return;

    _data[x + y * _width] = value;
}


QImage AImage32::qpal() const
{
    QImage out(_width, _height, QImage::Format_ARGB32);

    QRgb colors[] =
    {
        qRgb(0xFF, 0xFF, 0xFF),
        qRgb(0xFF, 0x00, 0x00),
        qRgb(0x80, 0x00, 0x00),
        qRgb(0xFF, 0xFF, 0x00),
        qRgb(0x80, 0x80, 0x00),
        qRgb(0x00, 0xFF, 0x00),
        qRgb(0x00, 0x80, 0x00),
        qRgb(0x00, 0xFF, 0xFF),
        qRgb(0x00, 0x80, 0x80),
        qRgb(0x00, 0x00, 0xFF),
        qRgb(0x00, 0x00, 0x80),
        qRgb(0xFF, 0x00, 0xFF),
        qRgb(0x80, 0x00, 0x80),
        qRgb(0xC0, 0xC0, 0xC0),
        qRgb(0x80, 0x80, 0x80),
    };

    QRgb outer_color = qRgb(60, 60, 60);


    for(int y=0; y<_height; y++)
    {
        for(int x=0; x<_width; x++)
        {
            auto d = _data[x+y*_width];
            QRgb color = qRgb(0,0,0);
            if ( d > 0)
                color = colors[(d - 1) % (sizeof(colors)/sizeof(colors[0]))];
            if ( d == (uint32_t)-1)
                color = outer_color;

            out.setPixel(x, y, color);
        }
    }

    return std::move(out);
}
