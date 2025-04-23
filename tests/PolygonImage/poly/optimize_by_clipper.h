#pragma once

#define _USE_MATH_DEFINES
#include "clipper.hpp"
#include "image_border.h"

struct ImageBorderInt
{
    static const int mul_factor = 100;

    //Внешная граница полигона
    ClipperLib::Path border;

    //Внутренняя граница полигона
    ClipperLib::Paths holes;

    void from(const ImageBorderElem& elem);
    ImageBorderElem to();
};


class OptimizeByClipper
{
public:
    OptimizeByClipper();
    ~OptimizeByClipper();

    void optimize(const ImageBorderElem& elem_double);


    //Сваливаем все точки в одну буольшую кучу.
    //Так не будет возможности holes оптимизировать, но зато будет сильно надёжнее
    void optimize(const std::vector<ImageBorderElem>& elems_double);

    std::vector<ImageBorderElem> result;
protected:
    ClipperLib::Path boundBox(const ClipperLib::Path& border);
    ClipperLib::Path clipByAngle(const ClipperLib::Path& cur_border, const ClipperLib::Path& original_border, double angle);

    //У нас могут быть несколько разных вариантов оптимизации.
    //Выбираем лучший.

    //Примитивнейший вариант, обрезает со всех сторон по кучи направлений
    //Если при обрезке слишком мало обрезалось, то её не применяем
    ClipperLib::Path optimizeUnsorted0(const ClipperLib::Path& unsorted_border);

    //Обрезаем только с четырёх направлений, но стараемся выбрать максимально удачный угол методом деления пополам.
    ClipperLib::Path optimizeUnsorted1(const ClipperLib::Path& unsorted_border);

    //Режем треугольником. Треугольник пытаемся соорудить так, что-бы он сильно порезал original_elems, но не порезал .
    ClipperLib::Path optimizeСoncaveSurface(const ClipperLib::Path& border);
    ClipperLib::Path optimizeСoncaveSurfaceSingleLine(const ClipperLib::Path& border,
                        ClipperLib::IntPoint p0, ClipperLib::IntPoint p1);

    //Оригинальные пиксели не должны быть затронуты нашей обрезкой.
    double clipOriginalDeltaArea(const ClipperLib::Path &clip_path);

    //Для отверстий, которые не слишком малы вставляем прямоугольную область максимально большую
    void addRectHoles(ImageBorderInt& out_border);
    void addRectHole(const ClipperLib::Path& hole, ImageBorderInt& out_border);
protected:
    //Изначальная граница, требуется для тестирования всякого
    std::vector<ImageBorderInt> original_elems;
};

/*
   !!!! OptimizeByClipper::clipByAngle разобраться со случаем когда solution возвращает несколько элементов.
*/
