#include "mainwindow.h"

#include <QApplication>
#include "atlas_stats.h"
#include <locale.h>

int main(int argc, char *argv[])
{
    setlocale(LC_NUMERIC, "C");
    QApplication a(argc, argv);

    if(0)
    {
        AtlasStats as;
        //as.load(R"(Z:\projects\tmp\polygon_atlas\эхо\out-tps\out.plist)");
        as.load(R"(Z:\projects\tmp\polygon_atlas\s2\tps\out.plist)");
        as.make_triangle_png("../out/out.png");
        as.save_areas("../out/out.txt");
        return 0;
    }

    //QString filename = R"(images\holes.png)";
    //QString filename = R"(images\hole.png)";
    //QString filename = R"(images\fill_outer.png)";
    //QString filename = R"(images\touched_areas.png)";
    //QString filename = R"(images\flag_1.png)";
    //QString filename = R"(images\soil_road_1.png)";
    //QString filename = R"(images\plants_forest_dk3.png)";
    //QString filename = R"(images\stone_forest_141_d.png)";
    //QString filename = R"(images\stone_95.png)";
    //QString filename = R"(images\xy.png)";
    //QString filename = R"(images\tile_floor_big3.png)";
    QString filename = R"(images\a0.png)";


    MainWindow w;
    w.load(filename);
    w.show();
    return a.exec();
}
