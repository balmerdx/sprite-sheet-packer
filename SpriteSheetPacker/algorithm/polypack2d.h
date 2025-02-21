/*
 Copyright (c) 2016, amakaseev < aleksey.makaseev.@gmail.com >
 All rights reserved.
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 1. Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef POLYPACK2D_H
#define POLYPACK2D_H

#include <QPointF>
#include <QDebug>
#include <QSize>
#include <math.h>
#include <functional>

namespace PolyPack2D {

    // TODO: move to math
    struct Point {
        float x, y;

        Point(): x(0), y(0) { }
        Point(float _x, float _y): x(_x), y(_y) { }
        Point(const Point& other): x(other.x), y(other.y) { }

        Point normalize() {
            float len = sqrtf(x*x + y*y);
            if (len != 0) {
                this->x /= len;
                this->y /= len;
            }
            return (*this);
        }

        Point operator + (const Point& p) const {
           return Point(this->x + p.x, this->y + p.y);
        }

        Point operator - (const Point& p) const {
           return Point(this->x - p.x, this->y - p.y);
        }

        Point operator * (float a) const {
           return Point(this->x * a, this->y * a);
        }
    };

    inline Point normal(const Point& a, const Point& b) {
        float dx = b.x - a.x;
        float dy = b.y - a.y;
        return Point(-dy, dx).normalize();
    }

    struct Rect {
        float left;
        float top;
        float right;
        float bottom;

        Rect operator + (const Rect& bRect) const {
           Rect aRect(*this);
           if (aRect.left > bRect.left) aRect.left = bRect.left;
           if (aRect.right < bRect.right) aRect.right = bRect.right;
           if (aRect.top > bRect.top) aRect.top = bRect.top;
           if (aRect.bottom < bRect.bottom) aRect.bottom = bRect.bottom;
           return aRect;
        }

        float width() const { return right - left; }
        float height() const { return bottom - top; }

        float area() const {
            return (right - left) * (bottom - top);
        }
    };

    inline bool operator == (const Rect& a, const Rect& b) {
        return ((a.left == b.left) && (a.top == b.top) && (a.right == b.right) && (a.bottom == b.bottom));
    }

    struct Triangles {
        std::vector<Point> verts;
        std::vector<unsigned short> indices;
    };

    bool rectIntersect(const Rect& r1, const Rect& r2);
    bool trianglesIntersect(const Triangles& a, const Triangles& b);
    /////


    template<class T> class Content {
    public:
        Content(const T &content, Triangles triangles, int border = 0)
        : _content(content)
        , _triangles(triangles)
        , _area(0)
        {
            _offset.x = _offset.y = 0;
            _bounds.left = _bounds.top = std::numeric_limits<float>::max();
            _bounds.right = _bounds.bottom = std::numeric_limits<float>::min();

            if (border) {
                // calculate normals
                std::vector<Point> norms(_triangles.verts.size());
                for (size_t i = 0; i < _triangles.indices.size(); i += 3) {
                    auto i1 = _triangles.indices[i + 0];
                    auto i2 = _triangles.indices[i + 1];
                    auto i3 = _triangles.indices[i + 2];

                    Point v1(_triangles.verts[i1]);
                    Point v2(_triangles.verts[i2]);
                    Point v3(_triangles.verts[i3]);

                    Point n1 = (normal(v1, v3) + normal(v2, v1)).normalize();
                    Point n2 = (normal(v2, v1) + normal(v3, v2)).normalize();
                    Point n3 = (normal(v3, v2) + normal(v1, v3)).normalize();

                    // smooth mormals
                    norms[i1] = ((norms[i1] + n1) * 0.5f).normalize();
                    norms[i2] = ((norms[i2] + n2) * 0.5f).normalize();
                    norms[i3] = ((norms[i3] + n3) * 0.5f).normalize();
                }
                // increase polygons with normals
                for (size_t v = 0; v < _triangles.verts.size(); ++v) {
                    _triangles.verts[v] = _triangles.verts[v] + norms[v] * border;
                }
            }

            // calculate bounding box
            for (auto point: _triangles.verts) {
                if (_bounds.left > point.x) _bounds.left = point.x;
                if (_bounds.right < point.x) _bounds.right = point.x;
                if (_bounds.top > point.y) _bounds.top = point.y;
                if (_bounds.bottom < point.y) _bounds.bottom = point.y;
            }

            setOffset(Point(-_bounds.left, -_bounds.top));
            _area = _bounds.area();
        }
        Content(const Content& other)
            : _content(other._content)
            , _offset(other.offset())
            , _triangles(other._triangles)
            , _area(other._area)
            , _bounds(other._bounds)
        {

        }

        const T& content() const { return _content; }
        double area() const { return _area; }
        const Point& offset() const { return _offset; }
        const Rect& bounds() const { return _bounds; }
        const Triangles& triangles() const { return _triangles; }

        void setOffset(const Point& offset) {
            _offset = offset;
            _bounds.left += offset.x;
            _bounds.right += offset.x;
            _bounds.top += offset.y;
            _bounds.bottom += offset.y;

            // translate triangles
            for (auto it_p = _triangles.verts.begin(); it_p != _triangles.verts.end(); ++it_p) {
                (*it_p).x += offset.x;
                (*it_p).y += offset.y;
            }
        }

    protected:
        T _content;
        Point _offset;
        Triangles _triangles;
        double _area;
        Rect _bounds;
    };

    template <class T> class ContentList: public std::vector<Content<T>> {
    public:
        ContentList<T>& operator += (const Content<T>& content) {
            this->push_back(content);
            return *this;
        }

        void sort() {
            std::sort(this->begin(), this->end(), [](const Content<T> &a, const Content<T> &b){
                auto areaA = a.area();
                auto areaB = b.area();
                return areaA > areaB;
            });
        }
    };

    template <class T> class Container: public std::vector<Content<T>> {
    public:
        void place(const ContentList<T>& inputContent, QSize sizeLimit = QSize(8192, 8192), int step = 5, std::function<void (int, int)> callback = NULL) {
            int contentIndex = 0;
            for (auto it = inputContent.begin(); it != inputContent.end(); ++it, ++contentIndex) {
                auto content = (*it);
                // insert first
                if (it == inputContent.begin()) {
                    _bounds = content.bounds();
                    _contentList.push_back(content);
                } else {
                    float startX = 0;//_bounds.left - (content.bounds().right - content.bounds().left) - step;
                    float startY = 0;//_bounds.top - (content.bounds().bottom - content.bounds().top) - step;
                    float endX = _bounds.right + step + (content.bounds().right - content.bounds().left);
                    float endY = _bounds.bottom + step + (content.bounds().bottom - content.bounds().top);

                    bool isPlaces = false;
                    float bestArea = 0;
                    Point bestOffset;

                    bool translateTriangles = false;
                    Triangles contentTriangles;

                    for (float y = startY; y < endY; y+= step) {
                        for (float x = startX; x < endX; x+= step) {
                            auto contentBounds = content.bounds();
                            contentBounds.left += x;
                            contentBounds.right += x;
                            contentBounds.top += y;
                            contentBounds.bottom += y;

                            auto newBounds(_bounds + contentBounds);
                            float area = newBounds.area();
                            if ((area > bestArea) && (isPlaces)) {
                                continue;
                            }
//                            if (newBounds.width() > (newBounds.height()*2)) continue;
//                            if (newBounds.height() > (newBounds.width()*2)) continue;
                            if (newBounds.width() > sizeLimit.width()) continue;
                            if (newBounds.height() > sizeLimit.height()) continue;

                            translateTriangles = false;

                            // test intersect intersection
                            bool intersect = false;
                            for (auto in_it = _contentList.begin(); in_it != _contentList.end(); ++in_it) {
                                if (rectIntersect(contentBounds, (*in_it).bounds())) {
                                    // translate triangles and test intersection
                                    if (!translateTriangles) {
                                        contentTriangles = content.triangles();
                                        for (auto it_p = contentTriangles.verts.begin(); it_p != contentTriangles.verts.end(); ++it_p) {
                                            (*it_p).x += x;
                                            (*it_p).y += y;
                                        }
                                        translateTriangles = true;
                                    }
                                    if (trianglesIntersect(contentTriangles, (*in_it).triangles())) {
                                        intersect = true;
                                        break;
                                    }
                                }
                            }

                            if (!intersect) {
                                if ((!isPlaces) || (area < bestArea)) {
                                    bestArea = area;
                                    bestOffset = Point(x, y);
                                    isPlaces = true;
                                }
                            }
                        }
                    }

                    if (isPlaces) {
                        qDebug() << "Placing: " << contentIndex << "/" << inputContent.size();
                        if (callback)
                            callback(contentIndex, inputContent.size());

                        content.setOffset(bestOffset);
                        if (_bounds.left > content.bounds().left) _bounds.left = content.bounds().left;
                        if (_bounds.right < content.bounds().right) _bounds.right = content.bounds().right;
                        if (_bounds.top > content.bounds().top) _bounds.top = content.bounds().top;
                        if (_bounds.bottom < content.bounds().bottom) _bounds.bottom = content.bounds().bottom;
                        _contentList.push_back(content);
                    } else {
                        qDebug() << "Not placed";
                    }
                }
            }
        }

        const Rect& bounds() const { return _bounds; }
        const ContentList<T>& contentList() const { return _contentList; }

    protected:
        Rect _bounds;
        ContentList<T> _contentList;
    };

}

#endif // POLYPACK2D_H
