#pragma once
#include "image_border.h"

/*
   Оптимизируем полигон по каким-то критериям.
   Количество точек должно уменьшиться, но при этом не сильно
   возрасти (или наоборот уменьшиться для дырок) площадь полигона.
   Естественно при этом все точки изначального изображения должны остаться
   внутри полигона (снаружи для holes).
*/
class OptimizePolygon
{
public:
    OptimizePolygon();

    void optimize(const ImageBorderElem& elem);

    ImageBorderElem result;
protected:
    //Самый простой вариант - фильтруем точки лежащие на горизонтальных или вертикальных отрезках.
    //Края отрезков естественно оставляем.
    //Эта функция удаляет часть точек, но при этом мы всё ещё идём точно по границе объекта.
    std::vector<p2t::Point> filterHorizontalVertical(const std::vector<p2t::Point>& border);

    //Фильтруем вогнутые части полигона.
    //Если точку можно удалить и при этом не обрезать полигон, то пользуемся этим.
    std::vector<p2t::Point> filterConcave(const std::vector<p2t::Point>& border);
};

