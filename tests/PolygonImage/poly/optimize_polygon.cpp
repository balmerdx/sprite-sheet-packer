#include "optimize_polygon.h"

OptimizePolygon::OptimizePolygon()
{

}


void OptimizePolygon::optimize(const ImageBorderElem& elem)
{
    //result = elem; return;
    result.border = filterHorizontalVertical(elem.border);

    for(int i=0; i<3; i++)
        result.border = filterConcave(result.border);
}

std::vector<p2t::Point> OptimizePolygon::filterHorizontalVertical(const std::vector<p2t::Point>& border)
{
    if (border.size() < 4)
        return border;

    const double eps = 0.001f;
    const double big = 0.1f;
    std::vector<p2t::Point> out;

    auto is_horizontal = [big, eps](const p2t::Point& p0, const p2t::Point& p1)
    {
        p2t::Point d = p1 - p0;
        return fabs(d.x) > big && fabs(d.y) < eps;
    };

    auto is_vertical = [big, eps](const p2t::Point& p0, const p2t::Point& p1)
    {
        p2t::Point d = p1 - p0;
        return fabs(d.x) < eps && fabs(d.y) > big;
    };

    bool last_horizontal = is_horizontal(border.back(), border[0]);
    bool last_vertical = is_vertical(border.back(), border[0]);

    for(size_t idx = 0; idx < border.size(); idx++)
    {
        p2t::Point p_cur = border[idx];
        p2t::Point p_next = border[(idx+1)%border.size()];
        bool cur_horizontal = is_horizontal(p_next, p_cur);
        bool cur_vertical = is_vertical(p_next, p_cur);

        if((last_horizontal && cur_horizontal) || (last_vertical && cur_vertical))
        {
            //skip this point
            continue;
        }

        out.push_back(p_cur);
        last_horizontal = cur_horizontal;
        last_vertical = cur_vertical;
    }

    return out;
}

std::vector<p2t::Point> OptimizePolygon::filterConcave(const std::vector<p2t::Point>& border)
{
    std::vector<p2t::Point> out;
    const double eps = 0.01f;

    bool add_next_anyway = false;
    for(size_t idx = 0; idx < border.size(); idx++)
    {
        p2t::Point p0 = border[(idx+border.size()-1)%border.size()];
        p2t::Point p1 = border[idx];
        p2t::Point p2 = border[(idx+1)%border.size()];

        //Расстояние до линии
        auto sign = p2t::Cross(p1-p0, p2-p0) / (p2-p0).Length();

        if (sign < eps && !add_next_anyway)
        {
            add_next_anyway = true;
            continue;
        }

        add_next_anyway = false;
        out.push_back(p1);
    }

    return out;

}
