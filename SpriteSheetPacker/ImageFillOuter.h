#pragma once

#include <QImage>

/*
   Берём прямоугольник и заполняем его вокруг пикселями с границы изображения.
   Угловые квалратики пока вообще не заполняем.
*/
void fillOuter(QImage& image, QRect sprite_size, int border_size);
