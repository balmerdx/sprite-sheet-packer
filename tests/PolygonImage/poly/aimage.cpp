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
    delete[] _data;
    _width = d._width;
    _height = d._height;
    _data = d._data;

    d._width = 0;
    d._height = 0;
    d._data = nullptr;
}

AImage AImage::clone() const
{
    AImage a(_width, _height);
    memcpy(a._data, _data, _width * _height * sizeof(_data[0]));
    return a;
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

AImage AImage::expandRightBottom(int border)
{
    assert(border >=0 && border < 100);
    AImage ex_image(_width + border, _height + border);

    for(int iy = 0; iy < _height; iy++)
    {
        for(int ix = 0; ix < _width; ix++)
        {
            auto v = get(ix, iy);
            if (!(v > 0))
                continue;
            ex_image.set(ix, iy, v);

            //fill right
            for(int border_idx = 0; border_idx < border; border_idx++)
            {
                int x = ix + 1 + border_idx;
                if(x < _width && get(x, iy) > 0)
                {
                    //Встретили непрозрачный пиксель, дальше не надо продолжать.
                    break;
                }

                ex_image.set(x, iy, v);
            }

            //fill bottom
            for(int border_idx = 0; border_idx < border; border_idx++)
            {
                int y = iy + 1 + border_idx;
                if(y < _height&& get(ix, y) > 0)
                {
                    //Встретили непрозрачный пиксель, дальше не надо продолжать.
                    break;
                }

                ex_image.set(ix, y, v);
            }
        }
    }

    return std::move(ex_image);
}

void AImage::excludeMask(const AImage& mask, uint8_t set_value)
{
    int xlimit = std::min(width(), mask.width());
    int ylimit = std::min(height(), mask.height());
    for(int y = 0; y < ylimit; y++)
    {
        for (int x = 0; x < xlimit; x++)
        {
            if(mask.get(x,y) > 0)
            {
                set(x, y, set_value);
            }
        }
    }
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
    delete[] _data;
    _width = d._width;
    _height = d._height;
    _data = d._data;

    d._width = 0;
    d._height = 0;
    d._data = nullptr;
}

AImage32 AImage32::clone() const
{
    AImage32 a(_width, _height);
    memcpy(a._data, _data, _width * _height * sizeof(_data[0]));
    return a;
}


AImage32::AImage32(int width, int height, uint32_t fill)
{
    _width = width;
    _height = height;
    _data = new uint32_t[_width*_height];
    std::fill_n(_data, _width*_height, fill);
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
