#pragma once
#include "aimage.h"
#include <QJsonObject>
#include <QJsonArray>

/*
   Изображение битовая маска, 1 бит на пиксель.
   Умеет быстро делать проверку, что одно изображение находится в пределах другого и там нет пересечения
   с заполненными пикселями.
   При уменьшении изображения пиксель считается заполненным, если хотя-бы одно из мест заполнено.
*/
class BinImage
{
public:
    BinImage(int width, int height, bool zero_at_end = true);

    //Создаёт и уменьшает изображения используя maxpool операцию
    //zero_at_end = true добавляет в конце один нулевой элемент.
    BinImage(const AImage32& img, int granularity_x, int granularity_y, bool zero_at_end = true);
    BinImage(const AImage& img, int granularity_x, int granularity_y, bool zero_at_end = true);

    //Сдвигает изображение по оси x вправо. При этом width = img + shift_x
    //Имеет смысл сдвигать в пределах байта, для того, что-бы быстро тестировать вхождение изображения в другое изображение
    BinImage(const BinImage& img, int shift_x);

    BinImage();
    virtual ~BinImage();

    BinImage(BinImage&& src);
    BinImage(const BinImage&) = delete;
    BinImage& operator= (const BinImage&) = delete;
    void operator=(BinImage&&);
    BinImage clone() const;

    int width() const { return _width; }
    int height() const { return _height; }
    bool in(int x, int y) const { return x >= 0 && x < _width && y >= 0 && y < _height; }

    //Зв переделами изображения падает по assert
    bool get(int x, int y) const;
    //За переделами изображения возвращает false
    bool getb(int x, int y) const;

    void set(int x, int y, bool value);

    QImage qimage(QRgb color_white = qRgb(0xFF, 0xFF, 0xFF), QRgb color_black = qRgb(0, 0, 0)) const;

    QJsonObject toJson();
protected:
    int calcStride(int width, bool zero_at_end);
    void init(int width, int height, bool zero_at_end);
protected:
    int _width = 0;
    int _height = 0;
    //Реальная длина одной строки (в uint64_t)
    int _stride = 0;
    uint64_t* _data = nullptr;

    friend class BinImageTest;
};


class BinImageTest
{
public:
    BinImageTest(const BinImage& img);
    ~BinImageTest();

    /*
       Тестирует, что BinImage переданная в конструкторе входит в big_image
       и при этом ни один из пикселей не пересекается.
       Старается делать это максимально быстро.
    */
    bool test(const BinImage& big_image, int x, int y);

    //Помещает текущее изображение на big_image
    void place(BinImage& big_image, int x, int y);
protected:
    //Изображеие предварительно сдвинутое на разное количество пикселей.
    //Требйется для быстрого теста.
    static const int image_mask_size = 8;
    BinImage img_mask[image_mask_size];
};
