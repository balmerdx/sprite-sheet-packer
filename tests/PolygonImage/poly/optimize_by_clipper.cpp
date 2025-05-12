#include "optimize_by_clipper.h"
#include "poly2tri.h"
#include <QDebug>

using namespace ClipperLib;

double OptimizeByClipper::min_factor(double delta_points, double delta_area)
{
    return delta_points + delta_area / ImageBorderInt::mul_factor / ImageBorderInt::mul_factor / _params.pixels_per_point;
}

cInt Cross(const IntPoint& a, const IntPoint& b)
{
    return a.X * b.Y - a.Y * b.X;
}

double Crossd(const IntPoint& a, const IntPoint& b)
{
    double ax = a.X, ay = a.Y;
    double bx = b.X, by = b.Y;
    return ax * by - ay * bx;
}

cInt Dot(const IntPoint& a, const IntPoint& b)
{
    return a.X * b.X + a.Y * b.Y;
}

double Dotd(const IntPoint& a, const IntPoint& b)
{
    double ax = a.X, ay = a.Y;
    double bx = b.X, by = b.Y;
    return ax * bx + ay * by;
}

double Dist(const IntPoint& a, const IntPoint& b)
{
    auto dx = b.X - a.X;
    auto dy = b.Y - a.Y;
    return sqrt(dx*dx + dy*dy);
}

double Length(const IntPoint& a)
{
    return sqrt(a.X*a.X + a.Y*a.Y);
}


inline IntPoint operator +(const IntPoint& a, const IntPoint& b)
{
    return IntPoint(a.X + b.X, a.Y + b.Y);
}

inline IntPoint operator -(const IntPoint& a, const IntPoint& b)
{
    return IntPoint(a.X - b.X, a.Y - b.Y);
}

inline IntPoint operator *(const IntPoint& a, cInt b)
{
    return IntPoint(a.X * b, a.Y * b);
}

inline IntPoint operator *(const IntPoint& a, double b)
{
    return IntPoint(lround(a.X * b), lround(a.Y * b));
}

inline IntPoint Normal(const IntPoint& a)
{
    return IntPoint(-a.Y, a.X);
}

//p0,p1 - линия
//pt - точка для которой измеряем расстояние до линии (расстояние имеет знак)
double DistToLine(const IntPoint& pt, const IntPoint& p0, const IntPoint& p1)
{
    return  Cross(pt-p0, p1-p0) / Dist(p0, p1);
}

//crossed == 0 не пересекается сегмент с линией (но при этом линия может слинией пересекаться)
//crossed == 1,-1 направление пересечения
IntPoint SegmentCrossLine(const IntPoint& segment0, const IntPoint& segment1, const IntPoint& line0, const IntPoint& line1, int& crossed)
{
    double d0 = DistToLine(segment0, line0, line1);
    double d1 = DistToLine(segment1, line0, line1);
    crossed = 0;

    if (d0 == 0)
    {
        if (d1 < 0)
            crossed = -1;
        if (d1 > 0)
            crossed = +1;
    }

    if (d1 == 0)
    {
        if (d0 < 0)
            crossed = +1;
        if (d0 > 0)
            crossed = -1;
    }

    if (d0 > 0 && d1 < 0)
        crossed = -1;
    if (d0 < 0 && d1 > 0)
        crossed = +1;

    //Считаем всё относительно line0, что-бы не было очень больших цифр
    IntPoint s0 = segment0 - line0;
    IntPoint s1 = segment1 - line0;
    IntPoint t1s = line1 - line0;
    IntPoint t0s = s1 - s0;
    double D = Crossd(t0s, t1s);
    double cs = Crossd(s0, s1);

    if (fabs(D) < 0.1)
    {
        //Параллельные линии
        return line0;
    }

    return t1s * (cs * -1/D) + line0;
}


void ImageBorderInt::from(const ImageBorderElem& elem)
{
    border.clear();
    holes.clear();

    auto convert = [](const p2t::Point& p) -> ClipperLib::IntPoint
    {
        ClipperLib::IntPoint o;
        o.X = lround(p.x*mul_factor);
        o.Y = lround(p.y*mul_factor);
        return o;
    };

    border.resize(elem.border.size());
    for(size_t i=0; i < elem.border.size(); i++)
    {
        border[i] = convert(elem.border[i]);
    }

    holes.resize(elem.holes.size());
    for(size_t ih=0; ih < elem.holes.size(); ih++)
    {
        auto& from = elem.holes[ih];
        auto& to = holes[ih];
        to.resize(from.size());
        for(size_t i=0; i < from.size(); i++)
        {
            to[i] = convert(from[i]);
        }
    }
}

ImageBorderElem ImageBorderInt::to()
{
    ImageBorderElem out;
    double mul = 1. / mul_factor;

    auto convert = [mul](const ClipperLib::IntPoint& p)
    {
        return p2t::Point(p.X * mul, p.Y * mul);
    };

    out.border.resize(border.size());
    for(size_t i=0; i < border.size(); i++)
    {
        out.border[i] = convert(border[i]);
    }

    out.holes.resize(holes.size());
    for(size_t ih=0; ih < holes.size(); ih++)
    {
        auto& from = holes[ih];
        auto& to = out.holes[ih];
        to.resize(from.size());
        for(size_t i=0; i < from.size(); i++)
        {
            to[i] = convert(from[i]);
        }
    }

    return out;
}


OptimizeByClipper::OptimizeByClipper()
{
}

OptimizeByClipper::~OptimizeByClipper()
{
}

void OptimizeByClipper::optimize(const std::vector<ImageBorderElem>& elems_double)
{
    ClipperLib::Path unsorted_border;


    for(const ImageBorderElem& elem_double : elems_double)
    {
        ImageBorderInt elem;
        elem.from(elem_double);

        Paths simply_border;
        SimplifyPolygon(elem.border, simply_border);
        if (simply_border.size() != 1)
        {
            qDebug() << "OptimizeByClipper::optimize unexpected result!!! size =" << simply_border.size();
        } else
        {
            elem.border = simply_border[0];
        }

        unsorted_border.insert(unsorted_border.end(), elem.border.begin(), elem.border.end());
        original_elems.push_back(std::move(elem));
    }


    ImageBorderInt out;

    if(_params.pack_to_rect)
    {
        out.border =  boundBox(unsorted_border);
        result.push_back(std::move(out.to()));
        return;
    }

    out.border =  optimizeUnsorted(unsorted_border);
    out.border = optimizeСoncaveSurface(out.border);

    addHoles(out);

    result.push_back(std::move(out.to()));
}

ClipperLib::Path OptimizeByClipper::boundBox(const ClipperLib::Path& border)
{
    cInt xmin, ymin, xmax, ymax;

    if (border.empty())
    {
        xmin = xmax = 0;
        ymin = ymax = 0;
    } else
    {
        xmin = xmax = border[0].X;
        ymin = ymax = border[0].Y;
        for(size_t i=1; i<border.size(); i++)
        {
            auto p = border[i];
            if (p.X < xmin)
                xmin = p.X;
            if (p.X > xmax)
                xmax = p.X;
            if (p.Y < ymin)
                ymin = p.Y;
            if (p.Y > ymax)
                ymax = p.Y;
        }
    }

    Path out = {{xmin, ymin}, {xmax, ymin}, {xmax, ymax}, {xmin, ymax}};
    return out;
}

ClipperLib::Path OptimizeByClipper::clipByAngle(const ClipperLib::Path& cur_border, const ClipperLib::Path& original_border, double angle)
{
    auto line_len = _params.line_len * ImageBorderInt::mul_factor;
    IntPoint line0(0,0);
    IntPoint line1(lroundf(line_len*cos(angle)), lroundf(line_len*sin(angle)));

    //Ищем минимальную точку
    IntPoint extreme;
    double extreme_dist = 0;
    bool first = true;
    for(auto p : original_border)
    {
        auto d = DistToLine(p, line0, line1);
        if (first)
        {
            extreme_dist = d;
            extreme = p;
            first = false;
        } else
        {
            if (d > extreme_dist)
            {
                extreme_dist = d;
                extreme = p;
            }
        }
    }

    //Делаем квадрат, которым режем
    IntPoint clip_n(line1.Y, -line1.X);

    Path clip_path = {
                      extreme - line1, extreme - line1 + clip_n,
                      extreme + line1 + clip_n, extreme + line1};

    Clipper clipper;
    clipper.AddPath(cur_border, ptSubject, true);
    clipper.AddPath(clip_path, ptClip, true);

    Paths solution;
    bool ok = clipper.Execute(ctDifference, solution);
    if (!ok)
    {
        solution.clear();
        solution.push_back(cur_border);
    }

    if (solution.empty())
        return Path();

    return solution[0];
}

ClipperLib::Path OptimizeByClipper::optimizeUnsorted(const ClipperLib::Path& unsorted_border)
{
    Path border =  boundBox(unsorted_border);

    for(int iangle = 0; iangle < 4; iangle++ )
    {
        float base_angle = iangle*M_PI/2;

        struct Data
        {
            double angle;
            double area;
            double delta_area;
            size_t points;
            int delta_points;

            //Меньше - лучше.
            double factor(OptimizeByClipper* clipper)
            {
                return clipper->min_factor(delta_points, delta_area);
            }
        };

        std::vector<Data> data;

        double prev_area = Area(border);
        const int angles2 = _params.clip_angles_count;
        for(int iangle2 = 0; iangle2 <= angles2; iangle2++)
        {
            Data d;
            d.angle = base_angle + iangle2*M_PI/(angles2*2);
            auto clipped_border = clipByAngle(border, unsorted_border, d.angle);
            d.area = Area(clipped_border);
            d.delta_area = d.area - prev_area;
            d.points = clipped_border.size();
            d.delta_points = (int)clipped_border.size() - (int)border.size();

            data.push_back(d);
        }

        //Ищем минимум по какому-то критерию.
        //Выбираем две соседние точки минимальные и там метод деления пополам используем
        size_t min_idx = 0;
        double min_factor = data[0].factor(this);
        for(size_t i = 0; i < data.size(); i++)
        {
            Data& d = data[i];
            auto f = d.factor(this);
            if (f < min_factor)
            {
                min_factor = f;
                min_idx = i;
            }
        }

        if (min_idx > 0 && min_idx+1 < data.size())
        {
            double fprev = data[min_idx-1].factor(this);
            double fnext = data[min_idx+1].factor(this);
            if (fprev < fnext)
            {
                min_idx--;
            }

        } else
        if (min_idx == 0)
        {
        } else
        if (min_idx+1 == data.size())
        {
            min_idx--;
        }

        double angle0 = data[min_idx].angle;
        double factor0 = data[min_idx].factor(this);
        double angle1 = data[min_idx+1].angle;
        double factor1 = data[min_idx+1].factor(this);

        double angle_mid = angle0;
        double f_mid = factor0;
        //Уточняем угол.
        for(int i=0; i<16; i++)
        {
            angle_mid = (angle0 + angle1) * 0.5;
            Data d;
            d.angle = angle_mid;
            auto clipped_border = clipByAngle(border, unsorted_border, d.angle);
            d.area = Area(clipped_border);
            d.delta_area = d.area - prev_area;
            d.points = clipped_border.size();
            d.delta_points = (int)clipped_border.size() - (int)border.size();

            f_mid = d.factor(this);
            if (factor0 < factor1)
            {
                factor1 = f_mid;
                angle1 = angle_mid;
            } else
            {
                factor0 = f_mid;
                angle0 = angle_mid;
            }
        }

        if (f_mid < 0)
        {
            border = clipByAngle(border, unsorted_border, angle_mid);
        }

    }

    return border;
}

double OptimizeByClipper::clipOriginalDeltaArea(const Path& clip_path)
{
    double delta_area = 0;
    for(const ImageBorderInt& elems : original_elems)
    {
        auto prev_area = Area(elems.border);

        Clipper clipper;
        clipper.AddPath(elems.border, ptSubject, true);
        clipper.AddPath(clip_path, ptClip, true);

        Paths solution;
        bool ok = clipper.Execute(ctDifference, solution);
        if (!ok)
        {
            return -1000;
        }

        double new_area = 0;
        for(const Path& p : solution)
        {
            new_area += Area(p);
        }

        delta_area += (new_area-prev_area);
    }

    return delta_area / ImageBorderInt::mul_factor / ImageBorderInt::mul_factor;
}

ClipperLib::Path OptimizeByClipper::optimizeСoncaveSurface(const ClipperLib::Path& border)
{
    struct Variants
    {
        //У нас есть несколько вариантов, из которых надо выбрать один, лучший
        std::vector<Path> variants;
    };

    const int variant_count = _params.edge_spit_variatns_count;
    std::vector<Variants> clip_triangles;
    for(size_t i=0; i<border.size(); i++)
    {
        clip_triangles.push_back(Variants());
        auto p0 = border[i];
        auto p1 = border[(i + 1)%border.size()];

        for(int iv=0; iv<variant_count; iv++)
        {
            double lerp_factor = (1 + iv) / (double)(variant_count+2);
            ClipperLib::Path out = findClipTriangle(p0, p1, lerp_factor);
            if (out.size() > 0)
            {
                clip_triangles.back().variants.push_back(out);
            }
        }
    }

    ClipperLib::Path out_border = border;
    for(const Variants& variants : clip_triangles)
    {
        bool found = false;
        Path min_solution;
        double min_min_factor = 0;
        for(const Path& clip_triangle : variants.variants)
        {
            if (clip_triangle.empty())
                continue;

            double delta_original = clipOriginalDeltaArea(clip_triangle);
            if (delta_original < -1)
                continue;

            Clipper clipper;
            clipper.AddPath(out_border, ptSubject, true);
            clipper.AddPath(clip_triangle, ptClip, true);

            Paths solution;
            bool ok = clipper.Execute(ctDifference, solution);
            if (!ok)
            {
                continue;
            }

            if (solution.size() != 1)
            {
                continue;
            }

            auto prev_area = Area(out_border);
            auto new_area = Area(solution[0]);
            int delta_points = (int)solution[0].size() - (int)out_border.size();
            auto delta_area = new_area - prev_area;
            auto cur_min_factor = min_factor(delta_points, delta_area);
            if (cur_min_factor >= 0)
                continue;

            if (!found || cur_min_factor < min_min_factor)
            {
                found = true;
                min_min_factor = cur_min_factor;
                min_solution = std::move(solution[0]);
            }
        }

        if (found)
        {
            out_border = std::move(min_solution);
        }
    }

    return out_border;
}

ClipperLib::Path OptimizeByClipper::findClipTriangle(ClipperLib::IntPoint p0, ClipperLib::IntPoint p1, double lerp_factor)
{
    Path out;

    IntPoint n = Normal(p1 - p0);
    //IntPoint pc = (p1 + p0) * 0.5;
    IntPoint pc = (p1 - p0) * lerp_factor + p0;
    IntPoint pn = pc + n;
    //Небольшой сдвиг на пол пикселя
    IntPoint nc = n * -(ImageBorderInt::mul_factor / Length(n) * 0.5);

    IntPoint cross_pt;
    IntPoint p0min;
    IntPoint p1min;
    bool p0p1min_found = false;

    {
        //Ищем точку пересечения отрезка и линии.
        //Выбираем из них ближайшую к линии.
        bool found = false;
        IntPoint near_cross_pt;
        //Индекс в original_elems
        size_t near_elem_idx = 0;
        //Индекс в original_elems[near_elem_idx].border
        //При пересечении у нас будут две точки original_elems[near_elem_idx].border[near_point_idx-1]
        // и original_elems[near_elem_idx].border[near_point_idx]
        size_t near_point_idx = 0;
        double near_dist = 0;

        for(size_t elem_idx = 0; elem_idx < original_elems.size(); elem_idx++)
        {
            ImageBorderInt& elem = original_elems[elem_idx];
            IntPoint prev_pt = elem.border.back();
            for(size_t point_idx = 0; point_idx < elem.border.size(); point_idx++)
            {
                const IntPoint& border_pt = elem.border[point_idx];
                int crossed;
                IntPoint pt = SegmentCrossLine(prev_pt, border_pt, pc, pn, crossed);
                if (crossed > 0)
                {
                    double cur_dist = DistToLine(pt, p1, p0);
                    if (!found || cur_dist > 0 && cur_dist < near_dist)
                    {
                        near_elem_idx = elem_idx;
                        near_point_idx = point_idx;
                        near_cross_pt = pt;
                        near_dist = cur_dist;
                        found = true;
                    }
                }

                prev_pt = border_pt;
            }
        }

        near_cross_pt = near_cross_pt - n * (ImageBorderInt::mul_factor / Length(n) * _params.edge_split_point_offset);

        if(!found)
            return Path();

        //Из нашего равнобедренного треугольника делаем неравнобедренный.
        //Проходимся по всем точкам border на которой у нас находится near_cross_pt
        //Смотрим с какой стороны она от линии pc, pn - это линия которая делит поровну наш
        //равнобедренный треугольник (near_cross_pt, p0nc, p1nc)
        //Проводя из near_cross_pt мы смотрим, где они пересекаются с линией p0nc, p1nc и соответственно модифицируем треугольник
        IntPoint p0nc = p0 + nc;
        IntPoint p1nc = p1 + nc;
        //double p0sign = DistToLine(p0, pc, pn);
        //double p1sign = DistToLine(p1, pc, pn);
        double p0dtl_min = 0;
        bool p0dtl_found = false;
        double p1dtl_min = 0;
        bool p1dtl_found = false;

        /*
        std::vector<IntPoint> tst = { IntPoint(127,133), IntPoint(61,133) };
        for(IntPoint& pt : tst)
        {
            pt.X *= ImageBorderInt::mul_factor;
            pt.Y *= ImageBorderInt::mul_factor;
        }
        */

        for(const IntPoint& border_pt : original_elems[near_elem_idx].border)
        //for(const IntPoint& border_pt : tst)
        {
            double dtl = DistToLine(border_pt, pc, pn);
            if (fabs(dtl) < ImageBorderInt::mul_factor)
                continue;

            double sign = Dotd(border_pt - near_cross_pt, n);
            if (sign > 0)
               continue;

            //dtl < 0 - значит со стороны точки p0, иначе со стороны точки p1
            //Не учитываем crossed
            int crossed;
            IntPoint pt_cross = SegmentCrossLine(border_pt, near_cross_pt, p0nc, p1nc, crossed);

            double dtl_cross = DistToLine(pt_cross, pc, pn);

            if (dtl_cross < 0)
            {
                if (!p0dtl_found || fabs(dtl_cross) < p0dtl_min)
                {
                    p0min = pt_cross;
                    p0dtl_min = fabs(dtl_cross);
                    p0dtl_found = true;
                }
            } else
            {
                if (!p1dtl_found || fabs(dtl_cross) < p1dtl_min)
                {
                    p1min = pt_cross;
                    p1dtl_min = fabs(dtl_cross);
                    p1dtl_found = true;
                }
            }
        }

        cross_pt = near_cross_pt;
        p0p1min_found = p0dtl_found && p1dtl_found;
    }

    //Треугольник которым обрезвем.
    if(p0p1min_found)
    {
        //Уточнённый треугольник.
        //Он касается двумя сторонами (и точкой cross_pt) нашего полигона.
        out.push_back(p0min);
        out.push_back(p1min);
        out.push_back(cross_pt);
    } else
    {
        //Debug output У нас есть равнобедренный треугольник, который
        //одной из вершин дотрагивается до полигона. Возможно, что стороны пересекают
        out.push_back(p0 + nc);
        out.push_back(p1 + nc);
        out.push_back(cross_pt);
    }

    if(false)
    {
        //Тестовый вывод
        ImageBorderInt out_int;
        out_int.border =  out;
        if(!out.empty())
            result.push_back(std::move(out_int.to()));
    }

    return  out;
}

void OptimizeByClipper::addHoles(ImageBorderInt &out_border)
{
    for(const ImageBorderInt& elem : original_elems)
    {
        for(const Path& hole : elem.holes)
        {
            addHole(hole, out_border);
        }
    }
}

void OptimizeByClipper::addHole(const ClipperLib::Path& hole, ImageBorderInt &out_border)
{
    Path hole_clean;
    CleanPolygon(hole, hole_clean);

    double delta = - _params.hole_clipper_offset_delta * ImageBorderInt::mul_factor;

    ClipperOffset clip_offset;
    clip_offset.AddPath(hole, jtMiter, etClosedPolygon);

    Paths solution;
    clip_offset.Execute(solution, delta);

    for(Path& cur_path : solution)
    {
        //Берём несколько точек, которые по разным направлениям экстремальны и
        //только их оставляем в полигоне (очень неоптимально по скорости упрощения полигона)
        std::vector<IntPoint> min_points;
        int angles = _params.points_in_hole;
        for(int iangle=0; iangle < angles; iangle++)
        {
            auto angle = 2 * M_PI * iangle / angles;
            auto dx = cos(angle);
            auto dy = sin(angle);

            bool found = false;
            double min_dist = 0;
            IntPoint min_point;
            for(IntPoint& p : cur_path)
            {
                auto dist = p.X * dx + p.Y*dy;
                if(!found || dist < min_dist)
                {
                    min_dist = dist;
                    min_point = p;
                    found = true;
                }
            }

            if (found)
                min_points.push_back(min_point);
        }

        Path simply_path;
        for(IntPoint& p : cur_path)
        {
            for(IntPoint& inp : min_points)
            {
                if(inp==p)
                {
                    simply_path.push_back(p);
                    break;
                }
            }
        }

        cur_path = simply_path;
    }

    {
        //Оставляем только те, для которых min_factor < 0
        Paths solution_clean;
        for(Path& cur_path : solution)
        {
            auto area = Area(cur_path);
            auto d = min_factor(cur_path.size(), -area);
            if(d < 0)
            {
                solution_clean.push_back(std::move(cur_path));
            }
        }

        solution = std::move(solution_clean);
    }

    //Решение успешно, если у нас новая дыра полностью внутри старой дыры
    {
        Paths solution_check;
        Clipper clipper;
        clipper.AddPath(hole, ptClip, true);
        clipper.AddPaths(solution, ptSubject, true);
        clipper.Execute(ctDifference, solution_check);

        if (!solution_check.empty())
            return;
    }


    for(Path& pout : solution)
        out_border.holes.push_back(pout);
    //out_border.border = solution[0];
}
