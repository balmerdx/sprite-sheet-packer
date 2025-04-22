#include "atlas_stats.h"

#include <QDir>
#include <QDebug>
#include <QFile>
#include <QImage>
#include <QPainter>
#include "pugi_utils.h"

std::vector<AtlasTriangle> AtlasStatsElem::triangles()
{
    std::vector<AtlasTriangle> out;

    out.resize(indices.size()/3);

    for(int itri=0; itri < out.size(); itri++)
    {
        int i0 = indices[itri*3];
        int i1 = indices[itri*3+1];
        int i2 = indices[itri*3+2];
        AtlasTriangle& t = out[itri];
        t.v[0] = vertices[i0];
        t.v[1] = vertices[i1];
        t.v[2] = vertices[i2];
    }

    return std::move(out);
}

double AtlasTriangle::area()
{
    double dx1 = v[1].x - v[0].x;
    double dy1 = v[1].y - v[0].y;

    double dx2 = v[2].x - v[0].x;
    double dy2 = v[2].y - v[0].y;

    return fabs(dx1 * dy2 - dy1 * dx2) * 0.5;
}

AtlasStats::AtlasStats()
{
}


AtlasStats::~AtlasStats()
{
}


bool AtlasStats::load(QString atlas_filename)
{
    pugi::xml_document pugi_doc;
    pugi::xml_parse_result result = pugi_load_xml(pugi_doc, atlas_filename);

    if (result.status != pugi::status_ok)
    {
        qDebug() << "Cannot load file:" << atlas_filename;
        qDebug() << result.description();
        return false;
    }

    this->atlas_filename = atlas_filename;
    atlas_elems.clear();

    pugi::xml_node pugi_root = pugi_doc.child("plist");
    pugi::xml_node pugi_dict = pugi_root.child("dict");
    for(pugi::xml_node pugi_dchild : pugi_dict)
    {
        if(strcmp(pugi_dchild.name(),"key")==0 && strcmp(pugi_dchild.text().get(),"frames")==0)
        {
            pugi::xml_node pugi_frames = pugi_dchild.next_sibling();

            for(pugi::xml_node pugi_frame : pugi_frames)
            {
                if (strcmp(pugi_frame.name(), "key")==0)
                {
                    AtlasStatsElem elem;
                    elem.filename = pugi_frame.text().get();
                    //qDebug() << elem.filename;

                    pugi::xml_node pugi_frame_dict = pugi_frame.next_sibling();
                    for(pugi::xml_node pugi_data : pugi_frame_dict)
                    {
                        if (strcmp(pugi_data.name(), "key")==0)
                        {
                            if(strcmp(pugi_data.text().get(), "triangles")==0)
                            {
                                //qDebug() << "tri" <<pugi_data.next_sibling().text().get();
                                QString triangles = pugi_data.next_sibling().text().get();
                                QStringList str_idx = triangles.split(' ');
                                for(QString& s : str_idx)
                                {
                                    bool ok;
                                    auto value = s.toInt(&ok);
                                    if (!ok)
                                        qDebug() << "Cannot parse" << s;
                                    elem.indices.push_back(value);
                                }
                            }

                            if(strcmp(pugi_data.text().get(), "verticesUV")==0)
                            {
                                //qDebug() << "uv" <<pugi_data.next_sibling().text().get();
                                QString triangles = pugi_data.next_sibling().text().get();
                                QStringList str_idx = triangles.split(' ');

                                elem.vertices.resize(str_idx.size()/2);
                                for(qsizetype i=0; i<str_idx.size(); i++)
                                {
                                    QString s = str_idx[i];
                                    bool ok;
                                    auto value = s.toInt(&ok);
                                    if (!ok)
                                        qDebug() << "Cannot parse" << s;

                                    if (i&1)
                                        elem.vertices[i/2].y = value;
                                    else
                                        elem.vertices[i/2].x = value;
                                }
                            }
                        }
                    }

                    atlas_elems.push_back(elem);
                }
            }
        }
    }

    return true;
}

void AtlasStats::make_triangle_png(QString out_filename)
{
    QFileInfo atlas_fileinfo(atlas_filename);
    QString in_png_name = atlas_fileinfo.dir().filePath(atlas_fileinfo.baseName()+".png");
    qDebug() << in_png_name;

    QImage image;
    if (!image.load(in_png_name))
    {
        qDebug() << "Cannot load :" << in_png_name;
        return;
    }

    QPainter p(&image);
    p.setPen(QColor(0, 255, 0));
    for(AtlasStatsElem& elem : atlas_elems)
    {
        std::vector<AtlasTriangle> triangles = elem.triangles();
        for(auto t : triangles)
        {
            TriangleVertices v0 = t.v[0];
            TriangleVertices v1 = t.v[1];
            TriangleVertices v2 = t.v[2];

            p.drawLine(v0.x, v0.y, v1.x, v1.y);
            p.drawLine(v1.x, v1.y, v2.x, v2.y);
            p.drawLine(v2.x, v2.y, v0.x, v0.y);
        }
    }

    p.end();

    if (!image.save(out_filename))
    {
        qDebug() << "Cannot save image :" << out_filename;
    }
}

void AtlasStats::save_areas(QString out_filename)
{
    QFile file(out_filename);
    if(!file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate))
    {
        qDebug() << "Cannot write :" << out_filename;
        return;
    }

    QTextStream out(&file);

    for(AtlasStatsElem& elem : atlas_elems)
    {
        std::vector<AtlasTriangle> triangles = elem.triangles();
        double sum_area = 0;
        for(auto t : triangles)
        {
            sum_area += t.area();
        }

        out << elem.filename << " area=" << sum_area << "\n";
    }

}
