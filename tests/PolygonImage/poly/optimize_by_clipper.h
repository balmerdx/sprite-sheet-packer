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
    struct Parameters
    {
        //Пожалуй один из самых значимых параметров.
        //Сколько пикселей должно убраться из площади полигона, что-бы считалось
        //что имеет смысл добавить одну вершину к полигону.
        double pixels_per_point = 150;
        //На сколько отступать от края дыры внутрь,
        //что-бы получить полигон
        double hole_clipper_offset_delta = 3;

        //Длинна линии для clipByAngle
        //скорее всего не потребуется менять никогда
        double line_len = 1000.;

        //Количество углов, под которыми пытаемся обрезать в optimizeUnsorted (для каждого сегмента PI/2)
        //скорее всего не потребуется менять никогда
        int clip_angles_count = 8;

        //Количество вариантов, какими пытаемся сделать выемку на ребре полигона в  optimizeСoncaveSurface
        //скорее всего не потребуется менять никогда
        int edge_spit_variatns_count = 6;

        //На сколько мы отступаем от границы полигона в findClipTriangle что-бы мелкие неровности не мешались.
        //скорее всего не потребуется менять никогда
        double edge_split_point_offset = 1.;

        //Количество вершин в дыре в полигоне
        //скорее всего не потребуется менять никогда
        int points_in_hole = 8;

        //pack_to_rect=true мы не пакуем, просто ограничиваем прямоугольником и всё.
        bool pack_to_rect = false;
    };

    OptimizeByClipper();
    ~OptimizeByClipper();

    Parameters _params;

    void optimize(const std::vector<ImageBorderElem>& elems_double);

    std::vector<ImageBorderElem> result;
protected:
    ClipperLib::Path boundBox(const ClipperLib::Path& border);
    ClipperLib::Path clipByAngle(const ClipperLib::Path& cur_border, const ClipperLib::Path& original_border, double angle);

    //Обрезаем только с четырёх направлений, но стараемся выбрать максимально удачный угол методом деления пополам.
    ClipperLib::Path optimizeUnsorted(const ClipperLib::Path& unsorted_border);

    //Режем треугольником. Треугольник пытаемся соорудить так, что-бы он сильно порезал original_elems, но не порезал .
    ClipperLib::Path optimizeСoncaveSurface(const ClipperLib::Path& border);
    //Возвращает треугольник, который почти наверняка не пересекается с original_elems, но при этом вырезает часть площади
    ClipperLib::Path findClipTriangle(ClipperLib::IntPoint p0, ClipperLib::IntPoint p1, double lerp_factor = 0.5);

    //Оригинальные пиксели не должны быть затронуты нашей обрезкой.
    double clipOriginalDeltaArea(const ClipperLib::Path &clip_path);

    //Для отверстий, которые не слишком малы вставляем прямоугольную область максимально большую
    void addHoles(ImageBorderInt& out_border);
    void addHole(const ClipperLib::Path& hole, ImageBorderInt& out_border);

    //see Parameters::pixels_per_point
    double min_factor(double delta_points, double delta_area);
protected:
    //Изначальная граница, требуется для тестирования всякого
    std::vector<ImageBorderInt> original_elems;
};
