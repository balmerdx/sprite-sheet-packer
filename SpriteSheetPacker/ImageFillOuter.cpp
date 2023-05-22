#include "ImageFillOuter.h"


void fillOuter(QImage& image, QRect sprite_size, int border_size)
{
    Q_ASSERT(image.format() == QImage::Format_ARGB32 || image.format() == QImage::Format_RGBA8888);

    {//fill top
        int y_original = sprite_size.top();
        if(y_original >=0 && y_original < image.height())
        {
            int x_start = qMax(sprite_size.left(), 0);
            int x_end = qMin(sprite_size.left() + sprite_size.width(), image.width());
            int y_start = qMax(sprite_size.top() - border_size, 0);
            int y_end = qMin(sprite_size.top(), image.height());
            for(int y = y_start; y < y_end; y++)
                for(int x = x_start; x < x_end; x++)
                    image.setPixel(x, y, image.pixel(x, y_original));
        }
    }

    {//fill bottom
        int y_original = sprite_size.bottom();
        if(y_original >=0 && y_original < image.height())
        {
            int x_start = qMax(sprite_size.left(), 0);
            int x_end = qMin(sprite_size.left() + sprite_size.width(), image.width());
            int y_start = qMax(sprite_size.top() + sprite_size.height(), 0);
            int y_end = qMin(y_start + border_size, image.height());
            for(int y = y_start; y < y_end; y++)
                for(int x = x_start; x < x_end; x++)
                    image.setPixel(x, y, image.pixel(x, y_original));
        }
    }

    {//fill left
        int x_original = sprite_size.left();
        if(x_original >=0 && x_original < image.width())
        {
            int x_start = qMax(sprite_size.left() - border_size, 0);
            int x_end = qMin(sprite_size.left(), image.width());
            int y_start = qMax(sprite_size.top(), 0);
            int y_end = qMin(sprite_size.top() + sprite_size.height(), image.height());
            for(int y = y_start; y < y_end; y++)
                for(int x = x_start; x < x_end; x++)
                    image.setPixel(x, y, image.pixel(x_original, y));
        }
    }

    {//fill right
        int x_original = sprite_size.right();
        if(x_original >=0 && x_original < image.width())
        {
            int x_start = qMax(sprite_size.left() + sprite_size.width(), 0);
            int x_end = qMin(x_start + border_size, image.width());
            int y_start = qMax(sprite_size.top(), 0);
            int y_end = qMin(sprite_size.top() + sprite_size.height(), image.height());
            for(int y = y_start; y < y_end; y++)
                for(int x = x_start; x < x_end; x++)
                    image.setPixel(x, y, image.pixel(x_original, y));
        }
    }
}
