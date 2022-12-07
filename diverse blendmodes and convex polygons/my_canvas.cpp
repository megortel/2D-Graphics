#include <iostream>
#include <deque>

#include "GCanvas.h"
#include "GRect.h"
#include "GColor.h"
#include "GBitmap.h"
#include "GPixel.h"
#include "GPoint.h"
#include "GPaint.h"
#include "GBlendMode.h"
#include "GMath.h"

int dividePixel(int value) {
    return (((value + 128) * 257) >> 16);
}

int float2pixel(float value) {
    return floor(value * 255 + 0.5);
}

GPixel color2pixel(const GColor& c) {
    int a = float2pixel(c.a);
    int r = float2pixel(c.r * c.a);
    int g = float2pixel(c.g * c.a);
    int b = float2pixel(c.b * c.a);
    return GPixel_PackARGB(a, r, g, b);
}

/* Blend Modes */
GPixel kClear(const GPixel src, const GPixel dst) {
    return GPixel_PackARGB(0, 0, 0, 0);
}

GPixel kSrc(const GPixel src, const GPixel dst) {
    return src;
}

GPixel kDst(const GPixel src, const GPixel dst) {
    return dst;
}

GPixel kSrcOver(const GPixel src, const GPixel dst) { // S + (1-Sa)*D
    int sa = GPixel_GetA(src);
    int sr = GPixel_GetR(src);
    int sg = GPixel_GetG(src);
    int sb = GPixel_GetB(src);

    int da = GPixel_GetA(dst);
    int dr = GPixel_GetR(dst);
    int dg = GPixel_GetG(dst);
    int db = GPixel_GetB(dst);

    if (sa == 255 || da == 0) {
        return kSrc(src, dst);
    }

    int a = sa + dividePixel((255 - sa) * da);
    int r = sr + dividePixel((255 - sa) * dr);
    int g = sg + dividePixel((255 - sa) * dg);
    int b = sb + dividePixel((255 - sa) * db);

    return GPixel_PackARGB(a, r, g, b);
}

GPixel kDstOver(const GPixel src, const GPixel dst) { // D + (1- Da)*S
    int sa = GPixel_GetA(src);
    int sr = GPixel_GetR(src);
    int sg = GPixel_GetG(src);
    int sb = GPixel_GetB(src);

    int da = GPixel_GetA(dst);
    int dr = GPixel_GetR(dst);
    int dg = GPixel_GetG(dst);
    int db = GPixel_GetB(dst);

    if (da == 255 || sa == 0) {
        return kDst(src, dst);
    }

    int a = da + dividePixel((255 - da) * sa);
    int r = dr + dividePixel((255 - da) * sr);
    int g = dg + dividePixel((255 - da) * sg);
    int b = db + dividePixel((255 - da) * sb);

    return GPixel_PackARGB(a, r, g, b);
}

GPixel kSrcIn(const GPixel src, const GPixel dst) { //Da*S
    int sa = GPixel_GetA(src);
    int sr = GPixel_GetR(src);
    int sg = GPixel_GetG(src);
    int sb = GPixel_GetB(src);

    int da = GPixel_GetA(dst);

    if (da == 0 || sa == 0) {
        return kClear(src, dst);
    }

    int a = dividePixel(da * sa);
    int r = dividePixel(da * sr);
    int g = dividePixel(da * sg);
    int b = dividePixel(da * sb);

    return GPixel_PackARGB(a, r, g, b);
}

GPixel kDstIn(const GPixel src, const GPixel dst) { // Sa*D
    int sa = GPixel_GetA(src);

    int da = GPixel_GetA(dst);
    int dr = GPixel_GetR(dst);
    int dg = GPixel_GetG(dst);
    int db = GPixel_GetB(dst);

    if (sa == 0 || da == 0) {
        return kClear(src, dst);
    }

    int a = dividePixel(sa * da);
    int r = dividePixel(sa * dr);
    int g = dividePixel(sa * dg);
    int b = dividePixel(sa * db);

    return GPixel_PackARGB(a, r, g, b);
}

GPixel kSrcOut(const GPixel src, const GPixel dst) { //(1-Da)*S
    int sa = GPixel_GetA(src);
    int sr = GPixel_GetR(src);
    int sg = GPixel_GetG(src);
    int sb = GPixel_GetB(src);

    int da = GPixel_GetA(dst);

    if (da == 255 || sa == 0) {
        return kClear(src, dst);
    }

    int a = dividePixel((255 - da) * sa);
    int r = dividePixel((255 - da) * sr);
    int g = dividePixel((255 - da) * sg);
    int b = dividePixel((255 - da) * sb);

    return GPixel_PackARGB(a, r, g, b);
}

GPixel kDstOut(const GPixel src, const GPixel dst) { //(1-Sa)*D
    int sa = GPixel_GetA(src);

    int da = GPixel_GetA(dst);
    int dr = GPixel_GetR(dst);
    int dg = GPixel_GetG(dst);
    int db = GPixel_GetB(dst);

    if (sa == 255 || da == 0) {
        return kClear(src, dst);
    }

    int a = dividePixel((255 - sa) * da);
    int r = dividePixel((255 - sa) * dr);
    int g = dividePixel((255 - sa) * dg);
    int b = dividePixel((255 - sa) * db);

    return GPixel_PackARGB(a, r, g, b);
}

GPixel kSrcATop(const GPixel src, const GPixel dst) { //Da*S + (1-Sa)*D
    int sa = GPixel_GetA(src);
    int sr = GPixel_GetR(src);
    int sg = GPixel_GetG(src);
    int sb = GPixel_GetB(src);

    int da = GPixel_GetA(dst);
    int dr = GPixel_GetR(dst);
    int dg = GPixel_GetG(dst);
    int db = GPixel_GetB(dst);

    if (da == 0) {
        return kClear(src, dst);
    }

    int a = dividePixel(da * sa) + dividePixel((255 - sa) * da);
    int r = dividePixel(da * sr) + dividePixel((255 - sa) * dr);
    int g = dividePixel(da * sg) + dividePixel((255 - sa) * dg);
    int b = dividePixel(da * sb) + dividePixel((255 - sa) * db);

    return GPixel_PackARGB(a, r, g, b);
}

GPixel kDstATop(const GPixel src, const GPixel dst) { // Sa*D + (1-Da)*S
    int sa = GPixel_GetA(src);
    int sr = GPixel_GetR(src);
    int sg = GPixel_GetG(src);
    int sb = GPixel_GetB(src);

    int da = GPixel_GetA(dst);
    int dr = GPixel_GetR(dst);
    int dg = GPixel_GetG(dst);
    int db = GPixel_GetB(dst);

    if (sa == 0) {
        return kClear(src, dst);
    }

    int a = dividePixel(sa * da) + dividePixel((255 - da) * sa);
    int r = dividePixel(sa * dr) + dividePixel((255 - da) * sr);
    int g = dividePixel(sa * dg) + dividePixel((255 - da) * sg);
    int b = dividePixel(sa * db) + dividePixel((255 - da) * sb);

    return GPixel_PackARGB(a, r, g, b);
}

GPixel kXor(const GPixel src, const GPixel dst) { //(1 - Sa)*D + (1 - Da)*S
    int sa = GPixel_GetA(src);
    int sr = GPixel_GetR(src);
    int sg = GPixel_GetG(src);
    int sb = GPixel_GetB(src);

    int da = GPixel_GetA(dst);
    int dr = GPixel_GetR(dst);
    int dg = GPixel_GetG(dst);
    int db = GPixel_GetB(dst);

    if (sa == 255 || da == 0) {
        int a = dividePixel((255 - da) * sa);
        int r = dividePixel((255 - da) * sr);
        int g = dividePixel((255 - da) * sg);
        int b = dividePixel((255 - da) * sb);
        return GPixel_PackARGB(a, r, g, b);
    }

    else if (da == 255 || sa == 0) {
        int a = dividePixel((255 - sa) * da);
        int r = dividePixel((255 - sa) * dr);
        int g = dividePixel((255 - sa) * dg);
        int b = dividePixel((255 - sa) * db);
        return GPixel_PackARGB(a, r, g, b);
    }

    else {
        int a = dividePixel((255 - sa) * da) + dividePixel((255 - da) * sa);
        int r = dividePixel((255 - sa) * dr) + dividePixel((255 - da) * sr);
        int g = dividePixel((255 - sa) * dg) + dividePixel((255 - da) * sg);
        int b = dividePixel((255 - sa) * db) + dividePixel((255 - da) * sb);
        return GPixel_PackARGB(a, r, g, b);
    }
}


 GPixel blendModes(GPixel src, GPixel dst, GBlendMode mode) {
    switch (mode) {
    case GBlendMode::kClear: return kClear(src, dst);
    case GBlendMode::kSrc: return kSrc(src, dst);
    case GBlendMode::kDst: return kDst(src, dst);
    case GBlendMode::kSrcOver: return kSrcOver(src, dst);
    case GBlendMode::kDstOver: return kDstOver(src, dst);
    case GBlendMode::kSrcIn: return kSrcIn(src, dst);
    case GBlendMode::kDstIn: return kDstIn(src, dst);
    case GBlendMode::kSrcOut: return kSrcOut(src, dst);
    case GBlendMode::kDstOut: return kDstOut(src, dst);
    case GBlendMode::kSrcATop: return kSrcATop(src, dst);
    case GBlendMode::kDstATop: return kDstATop(src, dst);
    case GBlendMode::kXor: return kXor(src, dst);
    }
    return kSrc(src, dst);
}


/* Edges */
struct Edge {
    float m, currX, b;
    int bottom, top;

    void set(float slope, float x, float bvalue, int t, int bott) {
        m = slope;
        currX = x;
        b = bvalue;
        top = t;
        bottom = bott;
    }

    void set(GPoint left, GPoint right) {
        /* If left is actually right point then switch points */
        if (left.y() > right.y()) {
            GPoint tmp = right;
            right = left;
            left = tmp;
        }

        top = GRoundToInt(left.y());
        bottom = GRoundToInt(right.y());
        if (top == bottom) {
            /* horiztonal line wont hit pixel centers */
            return;
        }

        m = (right.x() - left.x()) / (right.y() - left.y());
        b = left.x() - (m * left.y());
        currX = left.x() + m * (top - left.y() + 0.5);
    }

    int calculateX(int y) {
        return GRoundToInt(m * (y + 0.5) + b);
    }
};

Edge* createEdge(GPoint left, GPoint right) {
    Edge* e = new Edge();
    e->set(left, right);

    return e;
}

void blit(float left, float right, float canvasEdge, std::deque<Edge>& edges) {

    GPoint newLeft = GPoint();
    newLeft.fX = canvasEdge;
    newLeft.fY = left;

    GPoint newRight = GPoint();
    newRight.fX = canvasEdge;
    newRight.fY = right;

    Edge* blitt = createEdge(newLeft, newRight);
    edges.push_back(*blitt);
}

void clip(GPoint left, GPoint right, GRect canvas, std::deque<Edge>& edges) {
    /* If left is actually right point then switch points */
    if (left.y() > right.y()) {
        std::swap(left, right);
    }

    /* if all edges are out of bounds/unreachable cause pixel center */
    if (left.y() <= canvas.top() && right.y() <= canvas.top() || left.y() >= canvas.bottom() && right.y() >= canvas.bottom() || GRoundToInt(left.y()) == GRoundToInt(right.y())) {
        return;
    }

    /* clip top */
    if (left.y() < canvas.top()) {
        float newt = left.x() - ((right.x() - left.x()) / (right.y() - left.y())) * left.y();
        left.fX = newt;
        left.fY = canvas.top();
    }

    /* clip bottom */
    if (right.y() > canvas.bottom()) {
        float b = left.x() - ((right.x() - left.x()) / (right.y() - left.y())) * left.y();
        float newx = (((right.x() - left.x()) / (right.y() - left.y())) * canvas.height()) + b;
        right.fX = newx;
        right.fY = canvas.bottom();
    }

    /* ensure that left is less than right */
    if (left.y() > right.y()) {
        std::swap(left, right);
    }


    /* blit or edge projections */

    if (left.x() < 0) {
        if (right.x() < 0) {
            left.fX = 0;
            left.fY = left.fY;
            right.fX = 0;
            right.fY = right.fY;
        }
        else {
            float b = left.x() - ((right.x() - left.x()) / (right.y() - left.y())) * left.y();
            float m = ((right.x() - left.x()) / (right.y() - left.y()));
            GPoint blit;
            blit.fX = 0;
            blit.fY = left.fY;
            left.fX = 0;
            left.fY = (0 - b) / m;
            Edge* edge = createEdge(left, blit);
            edges.push_back(*edge);
        }
    }

    if (right.x() > canvas.width()) {
        if (left.x() > canvas.width()) {
            left.fX = canvas.width();
            left.fY = left.fY;
            right.fX = canvas.width();
            right.fY = right.fY;
        }
        else {
            float b = left.x() - ((right.x() - left.x()) / (right.y() - left.y())) * left.y();
            float m = ((right.x() - left.x()) / (right.y() - left.y()));
            GPoint blit;
            blit.fX = canvas.width();
            blit.fY = right.fY;
            right.fX = canvas.width();
            right.fY = (canvas.width() - b) / m;
            Edge* edge = createEdge(right, blit);
            edges.push_back(*edge);
        }
    }

   
    Edge* e = createEdge(left, right);
    edges.push_back(*e);
}

bool lessThan(Edge e1, Edge e2) {
    /* sorts edges, decides which edge is less than the other */
    
    if (e1.top < e2.top) {
        return true;
    }
    if (e1.top > e2.top) {
        return false;
    }
    if (e1.currX < e2.currX) {
        return true;
    }
    if (e1.currX > e2.currX) {
        return false;
    }
    return e1.m < e2.m;
}

class my_canvas : public GCanvas {
public:
    my_canvas(const GBitmap& device) : fDevice(device) {}

    void drawPaint(const GPaint& paint) override {
        GPixel p = color2pixel(paint.getColor());

        int width = fDevice.width();
        int height = fDevice.height();
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                GPixel* addy = fDevice.getAddr(x, y);
                *addy = p;
            }
        }
    }

    /**
    *  Fill the convex polygon with the color, following the same "containment" rule as
    *  rectangles.
    */
    void drawConvexPolygon(const GPoint points[], int count, const GPaint& paint) {
        GRect canvas = GRect::WH(fDevice.width(), fDevice.height());
        std::deque<Edge> edges;

        for (int i = 0; i < count; i++) {
            clip(points[i], points[(i + 1) % count], canvas, edges);
        }

        std::sort(edges.begin(), edges.end(), lessThan);

        if (edges.size() == 0) {
            return;
        }

        
        float max = edges.back().bottom;

        Edge left = edges.front();
        edges.pop_front();
        Edge right = edges.front();
        edges.pop_front();

        float min = left.top;
        float leftX = left.currX;
        float rightX = right.currX;

        for (int i = min; i < max; i++) {
            /* Draw pixel row using drawRect method */
            GRect pixelRow = GRect::LTRB(leftX, i, rightX, i + 1);
            drawRect(pixelRow, paint);

            if (i >= left.bottom) {
                left = edges.front();
                edges.pop_front();
                leftX = left.currX;
            }
            else {
                leftX += left.m;
            }
            if (i >= right.bottom) {
                right = edges.front();
                edges.pop_front();
                rightX = right.currX;
            }
            else {
                rightX += right.m;
            }
        }
    }

    void drawRect(const GRect& rect, const GPaint& paint) override {
        /* clipping the rectange in bounds */
        int L = round(rect.fLeft);
        int R = round(rect.fRight);
        int T = round(rect.fTop);
        int B = round(rect.fBottom);


        L = std::max(L, 0);
        R = std::min(R, fDevice.width());
        T = std::max(T, 0);
        B = std::min(B, fDevice.height());

        /* for loops to form rectangle */
        GPixel src = color2pixel(paint.getColor());

        for (int y = T; y < B; y++) {
            for (int x = L; x < R; x++) {
                GPixel* p = fDevice.getAddr(x, y);
                *p = blendModes(src, *p, paint.getBlendMode());
                }
            }
    }

private:
    // Note: we store a copy of the bitmap
    const GBitmap fDevice;
};

std::unique_ptr<GCanvas> GCreateCanvas(const GBitmap& device) {
    return std::unique_ptr<GCanvas>(new my_canvas(device));
}

static void make_regular_poly(GPoint pts[], int count, float cx, float cy, float radius) {
    float angle = 0;
    const float deltaAngle = M_PI * 2 / count;

    for (int i = 0; i < count; ++i) {
        pts[i] = { cx + cos(angle) * radius, cy + sin(angle) * radius };
        angle += deltaAngle;
    }
}

std::string GDrawSomething(GCanvas* canvas, GISize dim) {
    GColor srccolor = GColor();
    srccolor.a = 0.8;
    srccolor.r = 0.8;
    srccolor.g = 0.8;
    srccolor.b = 0.8;

    GRect rect = GRect::XYWH(0, 0, 50, 50);

    GPaint paint = GPaint(srccolor);
    GPoint triangle1;
    triangle1.fX = 10;
    triangle1.fY = 50;
    GPoint triangle2;
    triangle2.fX = 65;
    triangle2.fY = 65; 
    GPoint triangle3;
    triangle3.fX = 10;
    triangle3.fY = 65;

    GPoint triangle[] = { triangle1, triangle2, triangle3 };
    GRect rect3 = GRect::XYWH(0, 0, 150, 150);
    GRect rect4 = GRect::XYWH(0, 0, 200, 200);
    GRect rect5 = GRect::XYWH(0, 0, 250, 250);

    GPoint storage[8];
    for (int count = 8; count >= 3; --count) {
        make_regular_poly(storage, count, 256, 256, count * 10 + 120);
        for (int i = 0; i < count; ++i) {
            storage[i].fX += 0;
            storage[i].fY += 0;
        }
        GColor c = GColor::RGBA(fabs(sin(count * 3)),
            fabs(sin(count * 5)),
            fabs(sin(count * 21)),
            0.8f);
        canvas->drawConvexPolygon(storage, count, GPaint(c));
    }
    canvas->drawConvexPolygon(triangle, 3, paint);

    return "shape mania";
}