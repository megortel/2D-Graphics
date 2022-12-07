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
#include "GPath.h"

#include "blendModes.h"
#include "edges.h"


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
    bool w;
    if (left.y() > right.y()) {
        std::swap(left, right);
        w = false;
    }
    else {
        w = true;
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
            Edge* e = createEdge(p0, p1, w);
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
            Edge* e = createEdge(p0, p1, w);
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
            Edge* e = createEdge(p0, p1, w);
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
            Edge* e = createEdge(p1, p0, w);
            edges.push_back(*e);
        }
    }

    if (GRoundToInt(left.y()) != GRoundToInt(right.y())) {
        Edge* e = createEdge(left, right, w);
        edges.push_back(*e);
    }
    return edges;
}


class my_canvas : public GCanvas {
public:
    my_canvas(const GBitmap& device) : fDevice(device) {
        matrix = GMatrix();
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
        GMatrix& toSave = matrix;
        stack.push(toSave);
    }

    /**
    *  Copy the canvas state (CTM) that was record in the correspnding call to save() back into
    *  the canvas. It is an error to call restore() if there has been no previous call to save().
    */
    void restore() override {
        matrix = stack.top();
        assert(stack.size() >= 1);
        stack.pop();      
    }

    /**
    *  Modifies the CTM by preconcatenating the specified matrix with the CTM. The canvas
    *  is constructed with an identity CTM.
    *
    *  CTM' = CTM * matrix
    */
    void concat(const GMatrix& ctmMatrix) override {
        matrix = matrix * ctmMatrix;
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
            if (!shader->setContext(matrix)) {
                return;
            }
        }
        if (count < 0) {
            return;
        }

        GRect canvas = GRect::WH(fDevice.width(), fDevice.height());
        GPoint dstPoints[count];
        matrix.mapPoints(dstPoints, points, count);
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
            int leftX = std::max(0, GRoundToInt(left.currX));
            int rightX = std::min(fDevice.width(), GRoundToInt(right.currX));

            paintRow(min, leftX, rightX, paint);
            left.currX += left.m;
            right.currX += right.m;
            min++;
        }
    }


    void drawConvexPolygons(const GPoint points[], int count, const GPaint& paint) {
        GShader* shader = paint.getShader();
        if (shader != nullptr) {
            if (!shader->setContext(matrix)) {
                return;
            }
        }

        GRect canvas = GRect::WH(fDevice.width(), fDevice.height());
        GPoint dstPoints[count];
        matrix.mapPoints(dstPoints, points, count);
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
            int leftX = std::max(0, GRoundToInt(left.currX));
            int rightX = std::min(fDevice.width(), GRoundToInt(right.currX));

            blit(leftX, rightX, min, paint);
            left.currX += left.m;
            right.currX += right.m;
            min++;
        }
    }

    void drawRect(const GRect& rect, const GPaint& paint) override {
        GPoint p0 = GPoint();
        p0.fX = rect.fLeft;
        p0.fY = rect.fTop;
        GPoint p1 = GPoint();
        p1.fX = rect.fRight;
        p1.fY = rect.fTop;
        GPoint p2 = GPoint();
        p2.fX = rect.fRight;
        p2.fY = rect.fBottom;
        GPoint p3 = GPoint();
        p3.fX = rect.fLeft;
        p3.fY = rect.fBottom;

        GPoint points[4] = { p0, p1, p2, p3 };
        drawConvexPolygon(points, 4, paint);
    }

    void drawPath(const GPath& path, const GPaint& paint) override {
        GRect canvas = GRect::WH(fDevice.width(), fDevice.height());
        GPath gpath = path;
        GShader* shader = paint.getShader();
        if (shader != nullptr) {
            if (!(shader->setContext(matrix))) {
                return;
            }
        }
        gpath.transform(matrix);
        GPath::Edger edger = GPath::Edger(gpath);
        std::vector<Edge> edges;
        GPoint nextPts[2];

        while (edger.next(nextPts) != GPath::Verb::kDone) {
            std::vector<Edge> edgesAdd= clip(nextPts[0], nextPts[1], canvas);
            for (int i = 0; i < edgesAdd.size(); i++) {
                edges.push_back(edgesAdd.at(i));
            }
        }
        if (edges.size() == 0) {
            return;
        }

        std::sort(edges.begin(), edges.end(), lessThan);

        complexScan(edges, paint);
    }




private:
    // Note: we store a copy of the bitmap
    const GBitmap fDevice;

    std::stack<GMatrix> stack;
    GMatrix matrix; // identity matrix

    void paintRow(int y, int leftX, int rightX, const GPaint& paint) {
        leftX = std::max(0, leftX);
        rightX = std::min(fDevice.width(), rightX);
        bool srcover = false;

        blendProc blender = getProc(paint.getBlendMode());
        if (blender == kSrcOver) {
            srcover = true;
        }

        if (paint.getShader() == nullptr) {
            if (rightX <= leftX) {
                return;
            }
            GPixel src = colorToPixel(paint.getColor().pinToUnit());
            GPixel* addr = fDevice.getAddr(leftX, y);
            for (int x = leftX; x < rightX; x++) {
                if (srcover) {
                    *addr = kSrcOver(src, *addr);
                }
                else {
                    *addr = blender(src, *addr);
                }
                *addr++;
            }
        }
        else {
            GShader* shader = paint.getShader();
            if (!shader->setContext(matrix) || rightX <= leftX) {
                return;
            }
            int count = rightX - leftX;
            assert(leftX >= 0 && count >= 0);
            GPixel row[count];
            shader->shadeRow(leftX, y, count, row);

            GPixel* addr = fDevice.getAddr(leftX, y);

            for (int x = leftX; x < rightX; x++) {
                if (srcover) {
                    *addr = kSrcOver(row[x - leftX], *addr);
                }
                else {
                    *addr = blender(row[x - leftX], *addr);

                }
                *addr++;
            }
        }
    }

    void blit(int x0, int x1, int y, const GPaint& paint) {
        GShader* shader = paint.getShader();
        GPixel src = colorToPixel(paint.getColor());

        blendProc blender = getProc(paint.getBlendMode(), src);
        if (blender == kDst) {
            return;
        }
        bool srcover = (blender == kSrcOver);

        if (shader != nullptr) {
            if (!shader->setContext(matrix)) {
                return;
            }
            int count = x1 - x0;
            assert(x0 >= 0 && count >= 0); 
            GPixel row[count];
            shader->shadeRow(x0, y, count, row);
            
            for (int x = x0; x < x1; x++) {
                GPixel* addr = fDevice.getAddr(x, y);
                if (srcover) {
                    *addr = kSrcOver(row[x - x0], *addr); //pull ifs outside of loop and create 2 loops
                }
                else {
                    *addr = blender(row[x - x0], *addr);
                }
            }
        }
        else { //isOpague on shader
            if (srcover) {
                for (int x = x0; x < x1; x++) {
                    GPixel* addr = fDevice.getAddr(x, y);
                    *addr = kSrcOver(src, *addr);
                }
            } else if (blender == kSrc) {
                for (int x = x0; x < x1; x++) {
                    GPixel* addr = fDevice.getAddr(x, y);
                    *addr = kSrc(src, *addr);
                }
            }
            else {
                 for (int x = x0; x < x1; x++) {
                    GPixel* addr = fDevice.getAddr(x, y);
                    *addr = blender(src, *addr);
                 }
            }
        }

    }

    void complexScan(std::vector<Edge> edges, const GPaint& paint) {
        assert(edges.size() > 0);
        
        int x0;
        int x1;
        int y = edges[0].top;
        while (edges.size() > 0) {
            int index = 0;
            int w = 0;

            while (index < edges.size() && edges.at(index).top <= y) {
                if (w == 0) {
                    x0 = edges.at(index).calculateX(y);
                }

                w += edges.at(index).wind;

                if (w == 0) {
                    x1 = edges.at(index).calculateX(y);
                    blit(x0, x1, y, paint);
                }

                if (edges.at(index).isValid(y + 1) == false) {
                    edges.erase(edges.begin() + index);
                }
                else {
                    edges.at(index).currX += edges.at(index).m;
                    index++;
                }
            }

            y++;

            while (index < edges.size() && y == edges.at(index).top) {
                index++;
            }

            std::sort(edges.begin(), edges.begin() + index, edgeComparisonX);
        }
    }
};

std::unique_ptr<GCanvas> GCreateCanvas(const GBitmap& device) {
    return std::unique_ptr<GCanvas>(new my_canvas(device));
}

static void make_star(GPath* path, int count, float anglePhase) {
    assert(count & 1);
    float da = 2 * M_PI * (count >> 1) / count;
    float angle = anglePhase;
    for (int i = 0; i < count; i++) {
        GPoint p = { cosf(angle), sinf(angle) };
        i == 0 ? path->moveTo(p) : path->lineTo(p);
        angle += da;
    }
}

static void add_star(GPath* path, int count) {
    if (count & 1) {
        make_star(path, count, 0);
    }
    else {
        count >>= 1;
        make_star(path, count, 0);
        make_star(path, count, M_PI);
    }
}

std::string GDrawSomething(GCanvas* canvas, GISize dim) {
    GColor srccolor = GColor();
    srccolor.a = 0.8;
    srccolor.r = 0.8;
    srccolor.g = 0.8;
    srccolor.b = 0.8;

    canvas->clear({ 0, 0, 0.502, 1 });

    GMatrix scale = GMatrix::Scale(5, 5);
    GMatrix scale2 = GMatrix::Scale(15, 15);

    GPath path;;
    add_star(&path, 6);
    path.transform(scale);
    canvas->translate(60, 60);
    canvas->drawPath(path, GPaint({ 1, 1,.929, 1 }));

    path.reset();
    add_star(&path, 6);
    path.transform(scale);
    canvas->translate(15, 15);
    canvas->drawPath(path, GPaint({ 1, 1,.5, 1 }));

    path.reset();
    add_star(&path, 5);
    path.transform(scale);
    canvas->translate(20, 20);
    canvas->drawPath(path, GPaint({ 1, 1, 0, 1 }));

    path.reset();
    add_star(&path, 6);
    path.transform(scale);
    canvas->translate(10, 10);
    canvas->drawPath(path, GPaint({ 1, 1,.5, 1 }));

    path.reset();
    add_star(&path, 5);
    path.transform(scale);
    canvas->translate(10, 5);
    canvas->drawPath(path, GPaint({ 1, 1, 0, 1 }));

    path.reset();
    add_star(&path, 5);
    path.transform(scale);
    canvas->translate(7, 15);
    canvas->drawPath(path, GPaint({ 1, 1,.65, 1 }));

    path.reset();
    add_star(&path, 7);
    path.transform(scale);
    canvas->translate(26, 60);
    canvas->drawPath(path, GPaint({ 1, 1,.52, 1 }));

    path.reset();
    add_star(&path, 5);
    path.transform(scale2);
    canvas->translate(20, 30);
    canvas->drawPath(path, GPaint({ 1, 1,.32, 1 }));

    path.reset();
    add_star(&path, 5);
    path.transform(scale2);
    canvas->translate(-80, -75);
    canvas->drawPath(path, GPaint({ 1, 1,.78, 1 }));

    path.reset();
    add_star(&path, 7);
    path.transform(scale2);
    canvas->translate(-26, -60);
    canvas->drawPath(path, GPaint({ 1, 1,.32, 1 }));

    path.reset();
    add_star(&path, 7);
    path.transform(scale2);
    canvas->translate(100, 40);
    canvas->drawPath(path, GPaint({ 1, 1,.49, 1 }));

    return "shooting stars";
}
