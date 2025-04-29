#pragma once
#include "poly2tri.h"
#include "aimage.h"

/*
    Точки на границах полигона находятся на границе пикселей.
    К примеру 0, 0 находится слева-сверху от крайнего пикселя.
    К примеру 1, 0 находится справа-сверху от крайнего пикселя (и слева от следующего).
*/
struct ImageBorderElem
{
    //Внешная граница полигона
    std::vector<p2t::Point> border;

    //Внутренняя граница полигона
    std::vector<std::vector<p2t::Point> > holes;
};

struct ImageBorderParams
{
    uint8_t treshold = 5;

    //Если количество пикселей менее, чем poorly_visible_pixels_count
    //и их видимость менее poorly_visible_treshold, то такие области игнорируем.
    int poorly_visible_pixels_count = 8;
    uint8_t poorly_visible_treshold = 32;
};

class ImageBorder
{
public:
    //Внутри изображения могут быть несколько несвязанных областей,
    //в каждой из которых может быть несколько дыр
    //Соседями считаются точки по четырём направлениям.
    //Точки по диагонали не считаются соседними
    std::vector<ImageBorderElem> elems;

    ImageBorder();
    ~ImageBorder();

    void construct(const AImage& image, ImageBorderParams params);

    AImage32 colors;
    const uint32_t outer_color = (uint32_t)-1;

protected:
    void fillBorder(AImage& img, int xstart, int ystart, std::vector<p2t::Point>& border);

    //Заполняем внешнюю часть изображения белым цветом
    void fillOuter(AImage& img);

    //Заполняем пустые пиксели соприкасающиеся с границей как "внешнюю часть полигона"
    void fillOuterBorder(AImage& img, const AImage &original_img);

    void fillHoles(AImage& img);
protected:
    ImageBorderParams _params;
};
