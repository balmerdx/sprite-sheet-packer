#include "image_border.h"
#include <unordered_set>
#include <queue>
#include <functional>
#include <utility>
#include <QDebug>


struct FillIteratePoint
{
    int x, y;
};

namespace std {
template<>
struct hash<FillIteratePoint> {
    size_t operator()(const FillIteratePoint& p) const noexcept {
        return std::hash<uint64_t>{}(
            (uint32_t)p.x +
            (((uint64_t)(uint32_t)p.y) << 32));
/*
        // Combine hashes of x and y
        size_t h1 = hash<int>{}(p.x);
        size_t h2 = hash<int>{}(p.y);

        // Better hash combining than simple XOR
        // (This is similar to boost::hash_combine)
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
*/
    }
};
}

bool operator==(const FillIteratePoint& a, const FillIteratePoint& b) {
    return a.x == b.x && a.y == b.y;
}


//Ужасно неоптимальный вариант, но пускай такой будет
class FillIterate
{
public:
    //x,y - начальное значение для обхода полигона
    //fin - функция, которая для точки определяет - является ли точка того-же цвета, что и первоначальная точка
    //      если точка находится за пределами изображения, возвращаем false
    using eq_func = std::function<bool(int x, int y)>;

    FillIterate(int x, int y, eq_func fin);

    bool next(int& x, int& y);

    //Это граничный пиксел
    bool is_border(int x, int y);

protected:
    std::queue<FillIteratePoint> _cur_points;
    std::unordered_set<FillIteratePoint> _proceeded_points;
    eq_func _fin;
};

/*
*/
FillIterate::FillIterate(int x, int y, FillIterate::eq_func fin)
    : _fin(fin)
{
    if (_fin(x, y))
        _cur_points.push(FillIteratePoint{x,y});
}

bool FillIterate::next(int& x, int& y)
{
    if (_cur_points.empty())
        return false;

    auto p = _cur_points.front();
    x = p.x;
    y = p.y;
    _cur_points.pop();

    FillIteratePoint p4[4] = {
        {p.x-1, p.y},
        {p.x+1, p.y},
        {p.x, p.y-1},
        {p.x, p.y+1},
    };

    for(int i=0; i<4; i++)
    {
        FillIteratePoint& pp4 = p4[i];
        if(_proceeded_points.find(pp4)!=_proceeded_points.end())
            continue;
        _proceeded_points.insert(pp4);
        if (!_fin(pp4.x, pp4.y))
            continue;
        _cur_points.push(pp4);
    }

    return true;
}

bool FillIterate::is_border(int x, int y)
{
    FillIteratePoint p4[4] = {
                              {x-1, y},
                              {x+1, y},
                              {x, y-1},
                              {x, y+1},
                              };

    for(int i=0; i<4; i++)
    {
        FillIteratePoint& pp4 = p4[i];
        if (!_fin(pp4.x, pp4.y))
            return true;
    }
    return false;
}


/*
   Даже один пиксель будет иметь как минимум 4 точки границы.
   К примеру у нас есть пиксель (x,y) окруженный пустотой.
   Тогда в border = { {x,y}, {x+1, y}, {x+1, y+1}, {x, y+1}};
   Начальная точка обязана в 4 прилегающих пикселях иметь один полный и один пустой
*/
void ImageBorder::fillBorder(AImage& img, int xstart, int ystart, std::vector<p2t::Point>& border, bool border_is_hole)
{
    auto mask = [&img](int x, int y) -> uint8_t
    {
        uint8_t m =
            (img.getb(x-1, y-1) ? 1 : 0)
           +(img.getb(x-0, y-1) ? 2 : 0)
           +(img.getb(x-1, y-0) ? 4 : 0)
           +(img.getb(x-0, y-0) ? 8 : 0);
        return m;
    };

    border.clear();

    {
        auto m = mask(xstart, ystart);
        //У нас не выполняется первоначальное условие, выходим
        if (m == 0 || m == 15)
            return;
    }

    int xcur = xstart;
    int ycur = ystart;
    int xprev = xstart;
    int yprev = ystart;

    int dx_prev = 0;
    int dy_prev = 0;

    while(true)
    {
        int xnext = xcur;
        int ynext = ycur;
        //Двигаемся по часовой стрелке.
        auto m = mask(xcur, ycur);
        switch(m)
        {
        case 0:
            border.clear();
            break;
        case 1:
            //#.
            //..
            xnext--;
            break;
        case 2:
            //.#
            //..
            ynext--;
            break;
        case 3:
            //##
            //..
            xnext--;
            break;
        case 4:
            //..
            //#.
            ynext++;
            break;
        case 5:
            //#.
            //#.
            ynext++;
            break;
        case 6:
            //Тут надо учитывать предыдущий пиксель
            //В первоначальном пикселе такой конфигурации быть не может
            //.#
            //#.
            if (border_is_hole)
            {
                if (dx_prev == 1 && dy_prev == 0)
                {
                    ynext++;
                } else if (dx_prev == -1 && dy_prev == 0)
                {
                    ynext--;
                } else
                {
                    if(dx_prev == 0 && dy_prev == 0)
                    {
                        //В первоначальной конфигурации может быть, если мы обходим hole
                        ynext++;
                        break;
                    }

                    assert(0);
                    return;
                }
            } else
            {
                if (dx_prev == 1 && dy_prev == 0)
                {
                    ynext++;
                } else if (dx_prev == -1 && dy_prev == 0)
                {
                    ynext--;
                } else
                {
                    assert(0);
                    return;
                }
            }
            break;
        case 7:
            //##
            //#.
            ynext++;
            break;
        case 8:
            //..
            //.#
            xnext++;
            break;
        case 9:
            //Тут надо учитывать предыдущий пиксель
            //В первоначальном пикселе такой конфигурации быть не может
            //#.
            //.#
            if (border_is_hole)
            {
                if (dx_prev == 0 && dy_prev == 1)
                {
                    xnext++;
                } else if (dx_prev == 0 && dy_prev == -1)
                {
                    xnext--;
                } else
                {
                    assert(0);
                    return;
                }

            } else
            {
                if (dx_prev == 0 && dy_prev == 1)
                {
                    xnext--;
                } else if (dx_prev == 0 && dy_prev == -1)
                {
                    xnext++;
                } else
                {
                    assert(0);
                    return;
                }
            }
            break;
        case 10:
            //.#
            //.#
            ynext--;
            break;
        case 11:
            //##
            //.#
            xnext--;
            break;
        case 12:
            //..
            //##
            xnext++;
            break;
        case 13:
            //#.
            //##
            xnext++;
            break;
        case 14:
            //.#
            //##
            ynext--;
            break;
        case 15:
            border.clear();
            return;
        }

        assert(!(xprev==xnext && yprev==ynext));

        xprev = xcur;
        yprev = ycur;
        dx_prev = xnext - xcur;
        dy_prev = ynext - ycur;
        xcur = xnext;
        ycur = ynext;
        //qDebug() << "m =" << m << dx_prev << dy_prev;

        border.push_back(p2t::Point(xcur, ycur));

        if (xcur == xstart && ycur == ystart)
            break;
    }
}


ImageBorder::ImageBorder()
{
}

ImageBorder::~ImageBorder()
{
}

void ImageBorder::constructRect(const AImage& image)
{
    ImageBorderElem elem;
    elem.border.push_back(p2t::Point(0,0));
    elem.border.push_back(p2t::Point(image.width()-1, 0));
    elem.border.push_back(p2t::Point(image.width()-1, image.height()-1));
    elem.border.push_back(p2t::Point(0, image.height()-1));

    elems.clear();
    elems.push_back(elem);
}

void ImageBorder::construct(const AImage& image, ImageBorderParams params)
{
    /*
    Раскрашиваем изображение в N разных цветов.
    Для каждой области свой цвет.
    Так же раскрашиваем все дыры разными цветами.
    Дыры, которые соседствуют с границей изображения - являются внешней стороной объектов.
    */
    _params = params;

    AImage img(image.width(), image.height());
    colors = std::move(AImage32(image.width(), image.height()));

    auto fillImg = [&img, &image, treshold=_params.treshold] {
        for(int y=0; y<image.height(); y++)
            for(int x=0; x<image.width(); x++)
                if(image.get(x,y) > treshold)
                    img.set(x,y, 255);
    };

    fillImg();
    fillOuterBorder(img, image);
    fillImg();
    fillOuter(img);
    fillHoles(img);
}

void ImageBorder::fillOuterBorder(AImage& img, const AImage &original_img)
{
    uint32_t color_idx = 1;

    while(true)
    {
        int xfirst = -1, yfirst  = -1;

        for(int y=0; y<img.height(); y++)
        {
            for(int x=0; x<img.width(); x++)
            {
                if(img.get(x,y))
                {
                    xfirst = x;
                    yfirst = y;
                    break;
                }
            }

            if(img.in(xfirst, yfirst))
                break;
        }

        if(!img.in(xfirst, yfirst))
            break;

        ImageBorderElem elem;
        fillBorder(img, xfirst, yfirst, elem.border, false);

        FillIterate fi(xfirst, yfirst, [&img](int x, int y)
                       {
                           if (!img.in(x,y))
                               return false;
                           return img.get(x,y) != 0;
                       });

        int poorly_visible_pixels_count = 0;
        int good_visible_pixels_count = 0;
        while(true)
        {
            int x,y;
            if (!fi.next(x,y))
                break;

            if (original_img.get(x,y) > _params.poorly_visible_treshold)
                good_visible_pixels_count++;
            else
                poorly_visible_pixels_count++;
            //Стираем это изображение
            img.set(x, y, 0);
            //Раскрашиваем в разные цвета.
            colors.set(x,y, color_idx);
        }

        if (good_visible_pixels_count==0 && poorly_visible_pixels_count <= _params.poorly_visible_pixels_count)
            continue;

        elems.push_back(std::move(elem));
        color_idx++;
    }
}

void ImageBorder::fillOuter(AImage& img)
{
    while(true)
    {
        int xfirst = -1, yfirst  = -1;

        for(int x=0; x<img.width(); x++)
        {
            int y = 0;
            if(img.get(x, y) == 0)
            {
                xfirst = x;
                yfirst = y;
                break;
            }

            y = img.height()-1;
            if(img.get(x, y) == 0)
            {
                xfirst = x;
                yfirst = y;
                break;
            }
        }

        if(!img.in(xfirst, yfirst))
        for(int y=0; y<img.height(); y++)
        {
            int x = 0;
            if(img.get(x, y) == 0)
            {
                xfirst = x;
                yfirst = y;
                break;
            }

            x = img.width()-1;
            if(img.get(x, y) == 0)
            {
                xfirst = x;
                yfirst = y;
                break;
            }
        }

        if(!img.in(xfirst, yfirst))
            break;

        FillIterate fi(xfirst, yfirst, [&img](int x, int y)
           {
               if (!img.in(x,y))
                   return false;
               return img.get(x,y) == 0;
           });

        while(true)
        {
            int x,y;
            if (!fi.next(x,y))
                break;

            img.set(x, y, 255);
            //Раскрашиваем в разные цвета.
            colors.set(x,y, outer_color);
        }
    }

}

void ImageBorder::fillHoles(AImage& img)
{
    while(true)
    {
        int xfirst = -1, yfirst  = -1;
        uint32_t color_idx = outer_color;

        for(int y=0; y<img.height(); y++)
        {
            for(int x=0; x<img.width(); x++)
            {
                if(colors.get(x,y)!=0)
                    continue;

                FillIteratePoint p4[4] = {
                                          {x-1, y},
                                          {x+1, y},
                                          {x, y-1},
                                          {x, y+1},
                                          };

                for(int i=0; i<4; i++)
                {
                    FillIteratePoint& pp4 = p4[i];
                    auto bb = colors.getb(pp4.x, pp4.y);
                    if (bb ==0 || bb==outer_color)
                        continue;

                    color_idx = bb - 1;
                    xfirst = x;
                    yfirst = y;
                    break;
                }

                if(img.in(xfirst, yfirst))
                    break;
            }

            if(img.in(xfirst, yfirst))
                break;
        }

        if(!img.in(xfirst, yfirst))
            break;

        std::vector<p2t::Point> hole;
        fillBorder(img, xfirst, yfirst, hole, true);

        FillIterate fi(xfirst, yfirst, [this](int x, int y)
                       {
                           if (!colors.in(x,y))
                               return false;
                           return colors.get(x,y) == 0;
                       });

        while(true)
        {
            int x,y;
            if (!fi.next(x,y))
                break;

            //Раскрашиваем в разные цвета.
            colors.set(x, y, outer_color);
        }

        if (color_idx >= elems.size()) //!!!!Временно!!!!
            continue;

        elems[color_idx].holes.push_back(hole);
    }
}
