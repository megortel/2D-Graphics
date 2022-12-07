#include <algorithm>
#include <iostream>
#include <vector>
#include <stack>

#include "GCanvas.h"
#include "GRect.h"
#include "GColor.h"
#include "GBitmap.h"
#include "GPixel.h"
#include "GPoint.h"
#include "GPaint.h"
#include "GBlendMode.h"
#include "GMath.h"
#include "GShader.h"

#include "blendModes.h"
#include "edges.h"
#include "clip.h"


int floatToPixel(float value) {
    return floor(value * 255 + 0.5);
}

GPixel colorToPixel(const GColor& c) {
    int a = floatToPixel(c.a);
    int r = floatToPixel(c.r * c.a);
    int g = floatToPixel(c.g * c.a);
    int b = floatToPixel(c.b * c.a);
    return GPixel_PackARGB(a, r, g, b);
}

std::vector<Edge> clip(GPoint left, GPoint right, GRect canvas) {
    if (left.y() > right.y()) {
        std::swap(left, right);
    }

    std::vector<Edge> edges;

    /* if all edges are out of bounds/unreachable cause pixel center */
    if (right.y() <= canvas.top() || left.y() >= canvas.bottom()) {
        return edges;
    }

    /* clip top */
    if (left.y() < canvas.top()) {
        float newx = left.x() + (canvas.top() - left.y()) * ((right.x() - left.x()) / (right.y() - left.y()));
        left.fX = newx;
        left.fY = canvas.top();
    }

    /* clip bottom */
    if (right.y() > canvas.bottom()) {
        float newx = right.x() - (right.y() - canvas.bottom()) * (right.x() - left.x()) / (right.y() - left.y());
        right.fX = newx;
        right.fY = canvas.bottom();
    }

    /* ensure that left is less than right */
    if (left.x() > right.x()) {
        std::swap(left, right);
    }

    /* blit or edge projections */
    if (right.x() <= canvas.left()) {
        GPoint p0 = { canvas.left(), right.y() };
        GPoint p1 = { canvas.left(), left.y() };

        left.fX = canvas.left();
        right.fX = canvas.left();
        if (GRoundToInt(p0.y()) != GRoundToInt(p1.y())) {
            Edge* e = createEdge(p0, p1);
            edges.push_back(*e);
        }
        return edges;
    }

    if (left.x() >= canvas.right()) {
        GPoint p0 = { canvas.right(), right.y() };
        GPoint p1 = { canvas.right(), left.y() };

        left.fX = canvas.right();
        right.fX = canvas.right();
        if (GRoundToInt(p0.y()) != GRoundToInt(p1.y())) {
            Edge* e = createEdge(p0, p1);
            edges.push_back(*e);
        }
        return edges;
    }

    if (left.x() < canvas.left()) {
        GPoint p0 = { canvas.left(), left.y()};
        float newY = left.y() + (canvas.left() - left.x()) * (right.y() - left.y()) / (right.x() - left.x());
        GPoint p1 = { canvas.left(), newY };
        left.fX = canvas.left();
        left.fY = newY;
        if (GRoundToInt(p1.y()) != GRoundToInt(p0.y())) {
            Edge* e = createEdge(p0, p1);
            edges.push_back(*e);
        }

    }

    if (right.x() > canvas.right()) {
        GPoint p0 = { canvas.right(), right.y()};
        float newY = right.y() - (right.x() - canvas.right()) * (right.y() - left.y()) / (right.x() - left.x());
        GPoint p1 = { canvas.right(), newY };
        right.fX = canvas.right();
        right.fY = newY;
        if (GRoundToInt(p1.y()) != GRoundToInt(p0.y())) {
            Edge* e = createEdge(p1, p0);
            edges.push_back(*e);
        }
    }

    if (GRoundToInt(left.y()) != GRoundToInt(right.y())) {
        Edge* e = createEdge(left, right);
        edges.push_back(*e);
    }
    return edges;
}


class my_canvas : public GCanvas {
public:
    my_canvas(const GBitmap& device) : fDevice(device) {
        stack.push(matrix);
    }

    /**
    *  Save off a copy of the canvas state (CTM), to be later used if the balancing call to
    *  restore() is made. Calls to save/restore can be nested:
    *  save();
    *      save();
    *          concat(...);    // this modifies the CTM
    *          .. draw         // these are drawn with the modified CTM
    *      restore();          // now the CTM is as it was when the 2nd save() call was made
    *      ..
    *  restore();              // now the CTM is as it was when the 1st save() call was made
    */
    void save() override {
        GMatrix toSave = stack.top();
        //GMatrix copied(toSave[0], toSave[1], toSave[2], toSave[3], toSave[4], toSave[5]);
        stack.push(toSave);
    }

    /**
    *  Copy the canvas state (CTM) that was record in the correspnding call to save() back into
    *  the canvas. It is an error to call restore() if there has been no previous call to save().
    */
    void restore() override {
        matrix = stack.top();
        if (stack.size() >= 1) {
            stack.pop();
        }
       
    }

    /**
    *  Modifies the CTM by preconcatenating the specified matrix with the CTM. The canvas
    *  is constructed with an identity CTM.
    *
    *  CTM' = CTM * matrix
    */
    void concat(const GMatrix& ctmMatrix) override {
        stack.top().preConcat(ctmMatrix);
        //stack.top().Concat(stack.top(), ctmMatrix);
    }

    void drawPaint(const GPaint& paint) override {
        drawRect(GRect::WH(fDevice.width(), fDevice.height()), paint);
    }

    /**
    *  Fill the convex polygon with the color, following the same "containment" rule as
    *  rectangles.
    */
    void drawConvexPolygon(const GPoint points[], int count, const GPaint& paint) {
        GShader* shader = paint.getShader();
        if (shader != nullptr) {
            if (!(shader->setContext(matrix))) return;
        }

        if (count < 0) {
            return;
        }

        GRect canvas = GRect::WH(fDevice.width(), fDevice.height());
        GPoint dstPoints[count];
        stack.top().mapPoints(dstPoints, points, count);
        std::vector<Edge> edges;

        for (int i = 0; i < count; i++) {
            std::vector<Edge> edgesAdd = clip(dstPoints[i], dstPoints[(i + 1) % count], canvas);
            for (int j = 0; j < edgesAdd.size(); j++) {
                edges.push_back(edgesAdd.at(j));
            }

        }

        if (edges.size() == 0) {
            return;
        }
        std::sort(edges.begin(), edges.end(), lessThan);
        assert(edges.size() >= 2);

        // Set up our initial left and right boundary edges
        Edge left = edges.at(0);
        Edge right = edges.at(1);

        // Track index of next edge position
        int index = 2;
        float max = edges.at(edges.size() - 1).bottom;

        float min = left.top;

        int count1 = 0;
        int count2 = 0;
        for (auto e : edges) {
            if (e.bottom == max) {
                count1++;
            }
            if (e.top == min) {
                count2++;
            }
            assert(e.top < e.bottom);
        }
        assert(count1 == 2);
        assert(count2 == 2);

        while (min < max) {
            if (left.bottom <= min) {
                left = edges.at(index);
                index++;
            }
            if (right.bottom <= min) {
                right = edges.at(index);
                index++;
            }

            paintRow(min, GRoundToInt(left.currX), GRoundToInt(right.currX), paint);
            left.currX += left.m;
            right.currX += right.m;
            min++;
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

        GPoint p0 = GPoint();
        p0.fX = L;
        p0.fY = T;
        GPoint p1 = GPoint();
        p1.fX = R;
        p1.fY = T;
        GPoint p2 = GPoint();
        p2.fX = R;
        p2.fY = B;
        GPoint p3 = GPoint();
        p3.fX = L;
        p3.fY = B;

        GPoint points[4] = { p0, p1, p2, p3 };
        drawConvexPolygon(points, 4, paint);
    }


private:
    // Note: we store a copy of the bitmap
    const GBitmap fDevice;

    std::stack<GMatrix> stack;
    GMatrix matrix = GMatrix(); // identity matrix

    void paintRow(int y, int leftX, int rightX, const GPaint& paint) {
        leftX = std::max(0, leftX);
        rightX = std::min(fDevice.width(), rightX);

        blendProc blender = getProc(paint.getBlendMode());

        if (paint.getShader() == nullptr) {
            if (rightX <= leftX) {
                return;
            }
            GPixel src = colorToPixel(paint.getColor().pinToUnit());
            GPixel* addr = fDevice.getAddr(leftX, y);
            for (int x = leftX; x < rightX; x++) {
                *addr = blender(src, *addr);
                *addr++;
            }
        }
        else {
            GShader* shader = paint.getShader();
            if (!shader->setContext(stack.top()) || rightX <= leftX) {
                return;
            }
            int count = rightX - leftX;
            GPixel row[count];
            shader->shadeRow(leftX, y, count, row);

            GPixel* addr = fDevice.getAddr(leftX, y);
            for (int x = leftX; x < rightX; x++) {
                *addr = blender(row[x - leftX], *addr);                
                *addr++;
            }
        }
    }
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
    canvas->scale(10, 10);
    canvas->drawConvexPolygon(triangle, 3, paint);

    return "shape mania";
}
