#include "bin_image.h"

BinImage::BinImage()
{
}

BinImage::~BinImage()
{
    delete[] _data;
}

BinImage::BinImage(BinImage&& src)
{
    *this = std::move(src);
}

void BinImage::operator=(BinImage&& d)
{
    _width = d._width;
    _height = d._height;
    _stride = d._stride;
    _data = d._data;

    d._width = 0;
    d._height = 0;
    d._stride = 0;
    d._data = nullptr;
}


BinImage::BinImage(int width, int height, bool zero_at_end)
{
    init(width, height, zero_at_end);
}

void BinImage::init(int width, int height, bool zero_at_end)
{
    _width = width;
    _height = height;
    _stride = calcStride(width, zero_at_end);
    _data = new uint64_t[_stride * _height];
    memset(_data, 0, _stride * _height * sizeof(_data[0]));
}

int BinImage::calcStride(int width, bool zero_at_end)
{
    int bits_per_elem = sizeof(_data[0])*8;
    return (width + bits_per_elem - 1) / bits_per_elem + (zero_at_end ? 1 : 0);
}

BinImage::BinImage(const AImage32& img, int granularity_x, int granularity_y, bool zero_at_end)
{
    init((img.width() + granularity_x - 1) / granularity_x,
         (img.height() + granularity_y - 1) / granularity_y, zero_at_end);

    for(int y=0; y<_height; y++)
    {
        for(int x=0; x<_width; x++)
        {
            bool on = false;
            for(int yy=0; yy<granularity_y; yy++)
            {
                for(int xx=0; xx<granularity_x; xx++)
                {
                    auto d = img.getb(x*granularity_x+xx, y*granularity_y+yy);
                    if(d!=(uint32_t)-1 && d!=0)
                        on = true;
                }
            }

            //Не самый скоростной вариант заполнения
            set(x,y, on);
        }
    }
}

BinImage::BinImage(const AImage& img, int granularity_x, int granularity_y, bool zero_at_end)
{
    init((img.width() + granularity_x - 1) / granularity_x,
         (img.height() + granularity_y - 1) / granularity_y, zero_at_end);

    for(int y=0; y<_height; y++)
    {
        for(int x=0; x<_width; x++)
        {
            bool on = false;
            for(int yy=0; yy<granularity_y; yy++)
            {
                for(int xx=0; xx<granularity_x; xx++)
                {
                    auto d = img.getb(x*granularity_x+xx, y*granularity_y+yy);
                    if(d)
                        on = true;
                }
            }

            //Не самый скоростной вариант заполнения
            set(x,y, on);
        }
    }
}


BinImage::BinImage(const BinImage& img, int shift_x)
{
    init(img.width() + shift_x, img.height(), true);

    for(int y=0; y<img.height(); y++)
    {
        for(int x=0; x<img.width(); x++)
        {
            //Не самый скоростной вариант заполнения
            set(x+shift_x, y, img.get(x,y));
        }
    }
}

void BinImage::set(int x, int y, bool value)
{
    bool in = this->in(x, y);
    assert(in);
    if (!in)
        return;
    int xshift = x % (sizeof(_data[0])*8);
    auto& v = _data[x/(sizeof(_data[0])*8) + y * _stride];
    uint64_t mask = uint64_t(1) << xshift;
    if (value)
        v = v | mask;
    else
        v = v & ~mask;
}

bool BinImage::get(int x, int y) const
{
    bool in = this->in(x, y);
    assert(in);
    if (!in)
        return false;

    int xshift = x % (sizeof(_data[0])*8);
    auto& v = _data[x/(sizeof(_data[0])*8) + y * _stride];
    uint64_t mask = uint64_t(1) << xshift;

    return (v & mask) ? true : false;
}

bool BinImage::getb(int x, int y) const
{
    bool in = this->in(x, y);
    if (!in)
        return false;

    int xshift = x % (sizeof(_data[0])*8);
    auto& v = _data[x/(sizeof(_data[0])*8) + y * _stride];
    uint64_t mask = uint64_t(1) << xshift;

    return (v & mask) ? true : false;
}

QImage BinImage::qimage(QRgb color_white, QRgb color_black) const
{
    QImage out(_width, _height, QImage::Format_ARGB32);

    for(int y=0; y<_height; y++)
    {
        for(int x=0; x<_width; x++)
        {
            out.setPixel(x, y, get(x,y) ? color_white : color_black);
        }
    }
    return std::move(out);
}

/////////////////////BinImageTest///////////////////

BinImageTest::BinImageTest(const BinImage& img)
{
    for(int shift_x=0; shift_x<image_mask_size; shift_x++)
    {
        img_mask[shift_x] = std::move(BinImage(img, shift_x));
    }
}

BinImageTest::~BinImageTest()
{

}

bool BinImageTest::test(const BinImage& big_image, int x, int y)
{
    int x_shift = x%8;
    BinImage& mask = img_mask[x_shift];
    int bits_per_elem = sizeof(mask._data[0])*8;

    assert(x+mask.width() <= big_image.width()+bits_per_elem);
    assert(y+mask.height() <= big_image.height()+bits_per_elem);

    if(!(x+mask.width() <= big_image.width()+bits_per_elem))
        return false;
    if(!(y+mask.height() <= big_image.height()+bits_per_elem))
        return false;

    int x_start = (x - x_shift) / bits_per_elem;
    int x_end = (x - x_shift + mask.width() + bits_per_elem-1) / bits_per_elem;
    int x_size = x_end - x_start;

    //По хорошему тут align на sizeof(uint64_t) добавить надо!!!
    int big_start_size = x_size*sizeof(mask._data[0]);
    uint64_t* big_start_tmp = (uint64_t*)alloca(big_start_size);

    for (int iy = 0; iy < mask.height(); iy++)
    {
        uint64_t* big_start = big_image._data + (iy + y) * big_image._stride;
        uint64_t* mask_start = mask._data + iy * mask._stride;
        memcpy(big_start_tmp, (x/8)+(uint8_t*)big_start, big_start_size);

        for(int ix = 0; ix < x_size; ix++)
        {
            auto bs = big_start_tmp[ix];
            auto ms = mask_start[ix];
            if (big_start_tmp[ix] & mask_start[ix])
            {
                return false;
            }
        }
    }

    return true;
}

void BinImageTest::place(BinImage& big_image, int x, int y)
{
    int x_shift = x%8;
    BinImage& mask = img_mask[x_shift];
    int bits_per_elem = sizeof(mask._data[0])*8;

    assert(x+img_mask[0].width() <= big_image.width());
    assert(y+img_mask[0].height() <= big_image.height());


    int x_start = (x - x_shift) / bits_per_elem;
    int x_end = (x - x_shift + mask.width() + bits_per_elem-1) / bits_per_elem;
    int x_size = x_end - x_start;

    for (int iy = 0; iy < mask.height(); iy++)
    {
        uint64_t* big_start = big_image._data + (iy + y) * big_image._stride;
        uint64_t* mask_start = mask._data + iy * mask._stride;
        uint64_t* big_start_tmp = (uint64_t*)((x/8)+(uint8_t*)big_start);

        for(int ix = 0; ix < x_size; ix++)
        {
            big_start_tmp[ix] |= mask_start[ix];
        }
    }
}
