#include "mainwindow.h"
#include <QDebug>
#include <QPainter>

#include "poly/optimize_polygon.h"
#include "poly/optimize_by_clipper.h"
//#include "clipper.hpp"
//#include "poly2tri.h"

QImage createCheckmate(QSize size)
{
    QImage image(size, QImage::Format_RGB32);
    QRgb c0 = qRgb(103, 103, 103);
    QRgb c1 = qRgb(153, 153, 153);
    auto check_size = 10;
    for(int y=0; y<size.height(); y++)
    {
        for(int x=0; x<size.width(); x++)
        {
            QRgb p = (x/check_size+y/check_size)%2==0 ? c0 : c1;
            image.setPixel(x, y, p);
        }
    }
    return std::move(image);
}

void drawBorder(QPainter& p, const std::vector<p2t::Point>& border, int scale, QSize size)
{
    auto line = [&p, scale, size](p2t::Point p0, p2t::Point p1)
    {
        auto x0 = (int)lround(p0.x)*scale;
        auto y0 = (int)lround(p0.y)*scale;
        auto x1 = (int)lround(p1.x)*scale;
        auto y1 = (int)lround(p1.y)*scale;
        x0 = std::min(x0, size.width()-1);
        y0 = std::min(y0, size.height()-1);
        x1 = std::min(x1, size.width()-1);
        y1 = std::min(y1, size.height()-1);

        p.drawLine(x0, y0, x1, y1);
    };

    for(size_t cur = 1; cur < border.size(); cur++)
    {
        line(border[cur-1], border[cur]);
    }

    if(border.size() > 1)
    {
        line(border[border.size()-1], border[0]);
    }
}

void drawBorder(QPixmap& pixmap, const std::vector<ImageBorderElem>& elems, int scale)
{
    QPainter p(&pixmap);

    for(const ImageBorderElem& elem : elems)
    {
        p.setPen(QColor(0, 255, 0));
        auto& border = elem.border;
        drawBorder(p, border, scale, pixmap.size());

        p.setPen(QColor(0, 255, 255));
        qDebug() << "Holes count = " << elem.holes.size();
        for(auto& hole : elem.holes)
            drawBorder(p, hole, scale, pixmap.size());
        //drawBorder(p, elem.holes[3], scale, pixmap.size());
    }
}

//Кривущая функция, если исходный вектор поменяется/удалится,
//то указатели на точки станут невалидны
std::vector<p2t::Point*> makeTempPoints(const std::vector<p2t::Point>& p)
{
    std::vector<p2t::Point*> out(p.size());
    for(size_t i=0; i<p.size(); i++)
    {
        out[i] = const_cast<p2t::Point*>(&p[i]);
    }

    return std::move(out);
}


void drawTriangles(QPixmap& pixmap, const std::vector<ImageBorderElem>& elems, int scale)
{
    QPainter p(&pixmap);
    QSize size = pixmap.size();

    auto line = [&p, scale, size](p2t::Point p0, p2t::Point p1)
    {
        auto x0 = (int)lround(p0.x)*scale;
        auto y0 = (int)lround(p0.y)*scale;
        auto x1 = (int)lround(p1.x)*scale;
        auto y1 = (int)lround(p1.y)*scale;
        x0 = std::min(x0, size.width()-1);
        y0 = std::min(y0, size.height()-1);
        x1 = std::min(x1, size.width()-1);
        y1 = std::min(y1, size.height()-1);

        p.drawLine(x0, y0, x1, y1);
    };

    p.setPen(QColor(0, 255, 0));

    auto draw_border = [line](const std::vector<p2t::Point>& border)
    {
        std::vector<p2t::Point*> pborder = makeTempPoints(border);
        p2t::CDT cdt(pborder);

        cdt.Triangulate();
        std::vector<p2t::Triangle*> triangles = cdt.GetTriangles();
        qDebug() << "triangles =" << triangles.size() << " points =" << pborder.size();

        for(p2t::Triangle* t : triangles)
        {
            auto* p0 = t->GetPoint(0);
            auto* p1 = t->GetPoint(1);
            auto* p2 = t->GetPoint(2);
            line(*p0, *p1);
            line(*p1, *p2);
            line(*p2, *p0);
        }
    };

    if (true)
    {
        OptimizeByClipper op;
        op.optimize(elems);
        int cidx = 0;
        for(auto& r : op.result)
        {
            draw_border(r.border);
            p.setPen((cidx%2) ? QColor(255, 0, 0) : QColor(0, 0, 255));
            cidx++;
        }
    } else
    for(const ImageBorderElem& elem : elems)
    {
        OptimizeByClipper op;
        op.optimize(elem);
        for(auto& r : op.result)
            draw_border(r.border);
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{

    auto mainWidget = new QWidget();
    auto mainLayoput = new QHBoxLayout();
    labelImage = new QLabel();
    mainLayoput->addWidget(labelImage);
    mainWidget->setLayout(mainLayoput);

    addMenuElems();

    setCentralWidget(mainWidget);
    this->move(300, 0);
}

MainWindow::~MainWindow()
{
}

bool MainWindow::load(QString filename)
{
    this->filename = filename;

    QImage img;
    if (!img.load(filename))
    {
        qDebug() << "Cannot load " << filename;
        return false;
    }

    img = img.convertedTo(QImage::Format_ARGB32);

    bool draw_colors = true;

    ImageBorder ib;
    {
        ImageBorderParams params;
        //params.poorly_visible_pixels_count = 0; //Для теста не удаляем мелкие области
        ib.construct(img, params);
        //img = ib.tst.qalpha();
        //img = ib.tst.qgray();
        if (draw_colors)
            img = ib.colors.qpal();
    }


    auto checkmate = QPixmap::fromImage(createCheckmate(img.size()));

    {
        QPainter p(&checkmate);
        p.drawImage(0,0, img);
    }

    const int scale = 5;
    checkmate = checkmate.scaledToWidth(checkmate.width()*scale);

    if (drawType == DrawType::Polygon)
        drawBorder(checkmate, ib.elems, scale);
    else
        drawTriangles(checkmate, ib.elems, scale);

    labelImage->setPixmap(checkmate);

    return true;
}

void MainWindow::addMenuElems()
{
    QMenu* smenu = menuBar()->addMenu("Switch");

    actPolygon = smenu->addAction("Polygon", [this](){
        onSwitchDrawType(DrawType::Polygon);
        load(filename);
    });
    actPolygon->setCheckable(true);

    actTriangle = smenu->addAction("Triangle", [this](){
        onSwitchDrawType(DrawType::Triangle);
        load(filename);
    });
    actTriangle->setCheckable(true);

    onSwitchDrawType(DrawType::Triangle);
}


void MainWindow::onSwitchDrawType(DrawType dt)
{
    drawType = dt;
    actPolygon->setChecked(drawType==DrawType::Polygon);
    actTriangle->setChecked(drawType==DrawType::Triangle);
}
