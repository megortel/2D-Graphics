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
#include "my_composeShader.h"
#include "my_proxyShader.h"
#include "my_triShader.h"


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
            if (!(shader->setContext(matrix))) {
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

        //int count1 = 0;
        //int count2 = 0;
        //for (auto e : edges) {
        //    if (e.bottom == max) {
        //        count1++;
        //    }
        //    if (e.top == min) {
        //        count2++;
        //    }
        //    assert(e.top < e.bottom);
       // }
       // assert(count1 == 2);
       // assert(count2 == 2);

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
        GPoint nextPts[4];
        GPath::Verb nextVerb = edger.next(nextPts);

        while (nextVerb != GPath::Verb::kDone) {
            if (nextVerb == GPath::Verb::kLine) {
                std::vector<Edge> edgesAdd= clip(nextPts[0], nextPts[1], canvas);
                for (int i = 0; i < edgesAdd.size(); i++) {
                    edges.push_back(edgesAdd.at(i));
                }
            }
            else if(nextVerb == GPath::Verb::kQuad) {
                int n = numQuadSegments(nextPts);
                GPoint pt = nextPts[0];
                for (int i = 1; i < n; i++) {
                    float t = (float)i / n;
                    GPoint pt2 = getQuadPt(nextPts, t);
                    std::vector<Edge> edgesAdd = clip(pt, pt2, canvas);
                    for (int j = 0; j < edgesAdd.size(); j++) {
                        edges.push_back(edgesAdd.at(j));
                    }
                    pt = pt2;
                }
                std::vector<Edge> edgesAdd = clip(pt, nextPts[2], canvas);
                for (int k = 0; k < edgesAdd.size(); k++) {
                    edges.push_back(edgesAdd.at(k));
                }
            }
            else if (nextVerb == GPath::Verb::kCubic) {
                int n = numCubicSegments(nextPts);
                GPoint pt = nextPts[0];
                for (int i = 1; i < n; i++) {
                    float t = (float) i / n;
                    GPoint pt2 = getCubicPt(nextPts, t);
                    std::vector<Edge> edgesAdd = clip(pt, pt2, canvas);
                    for (int j = 0; j < edgesAdd.size(); j++) {
                        edges.push_back(edgesAdd.at(j));
                    }
                    pt = pt2;
                }
                std::vector<Edge> edgesAdd = clip(pt, nextPts[3], canvas);
                for (int k = 0; k < edgesAdd.size(); k++) {
                    edges.push_back(edgesAdd.at(k));
                }
            }
            else {
            }
            nextVerb = edger.next(nextPts); //increment
            
        }
        if (edges.size() == 0) {
            return;
        }

        std::sort(edges.begin(), edges.end(), lessThan);

        complexScan(edges, paint);
    }

    void drawTriangle(const GPoint pts[3], const GColor colors[], const GPoint texs[], const GPaint& paint) {
        if (colors != nullptr && texs != nullptr) {
            GPaint pnt(new my_compositeShader(new my_triShader(pts, colors), paint.getShader()));
            drawConvexPolygon(pts, 3, pnt);
        }
        else if (colors != nullptr) {
            GPaint pnt(new my_triShader(pts, colors));
            drawConvexPolygon(pts, 3, pnt);
        }
        else {
            drawConvexPolygon(pts, 3, paint);
        }
    }

    void drawTextTriangle(const GPoint pts[3], const GColor colors[], const GPoint texs[], GShader* ogshader) {
        GMatrix P;
        GMatrix T;
        GMatrix invertedT;

        P = GMatrix(pts[1].x() - pts[0].x(), pts[2].x() - pts[0].x(), pts[0].x(), pts[1].y() - pts[0].y(), pts[2].y() - pts[0].y(), pts[0].y());
        T = GMatrix(texs[1].x() - texs[0].x(), texs[2].x() - texs[0].x(), texs[0].x(), texs[1].y() - texs[0].y(), texs[2].y() - texs[0].y(), texs[0].y());

        if (T.invert(&invertedT) == false) {
            return;
            //do nothing
        }
        my_proxyShader prox = my_proxyShader(ogshader, P * invertedT);
        GPaint pnt(&prox);
        drawTriangle(pts, colors, texs, pnt);
    }


/**
 *  Draw a mesh of triangles, with optional colors and/or texture-coordinates at each vertex.
 *
 *  The triangles are specified by successive triples of indices.
 *      int n = 0;
 *      for (i = 0; i < count; ++i) {
 *          point0 = vertx[indices[n+0]]
 *          point1 = verts[indices[n+1]]
 *          point2 = verts[indices[n+2]]
 *          ...
 *          n += 3
 *      }
 *
 *  If colors is not null, then each vertex has an associated color, to be interpolated
 *  across the triangle. The colors are referenced in the same way as the verts.
 *          color0 = colors[indices[n+0]]
 *          color1 = colors[indices[n+1]]
 *          color2 = colors[indices[n+2]]
 *
 *  If texs is not null, then each vertex has an associated texture coordinate, to be used
 *  to specify a coordinate in the paint's shader's space. If there is no shader on the
 *  paint, then texs[] should be ignored. It is referenced in the same way as verts and colors.
 *          texs0 = texs[indices[n+0]]
 *          texs1 = texs[indices[n+1]]
 *          texs2 = texs[indices[n+2]]
 *
 *  If both colors and texs[] are specified, then at each pixel their values are multiplied
 *  together, component by component.
 */
    void drawMesh(const GPoint verts[], const GColor colors[], const GPoint texs[], int count, const int indices[], const GPaint& paint) {
        int num = 0;
        for (int i = 0; i < count; i++) {
            const GPoint pts[3] = { verts[indices[num + 0]], verts[indices[num + 1]], verts[indices[num + 2]] };

            if (colors != nullptr && texs != nullptr) {
                const GColor color[3] = { colors[indices[num + 0]], colors[indices[num + 1]], colors[indices[num + 2]] };
                const GPoint texture[3] = { texs[indices[num + 0]], texs[indices[num + 1]], texs[indices[num + 2]] };
                drawTextTriangle(pts, color, texture, paint.getShader());
            }
            else if (colors != nullptr) {
                const GColor color[3] = { colors[indices[num + 0]], colors[indices[num + 1]], colors[indices[num + 2]] };
                drawTriangle(pts, color, nullptr, paint);
            }
            else if (texs != nullptr) {
                const GPoint texture[3] = { texs[indices[num + 0]], texs[indices[num + 1]], texs[indices[num + 2]] };
                drawTextTriangle(pts, nullptr, texture, paint.getShader());
            }
            else {
                drawTriangle(pts, nullptr, nullptr, paint);                
            }
            num += 3;
        }
    }

    /**
 *  Draw the quad, with optional color and/or texture coordinate at each corner. Tesselate
 *  the quad based on "level":
 *      level == 0 --> 1 quad  -->  2 triangles
 *      level == 1 --> 4 quads -->  8 triangles
 *      level == 2 --> 9 quads --> 18 triangles
 *      ...
 *  The 4 corners of the quad are specified in this order:
 *      top-left --> top-right --> bottom-right --> bottom-left
 *  Each quad is triangulated on the diagonal top-right --> bottom-left
 *      0---1
 *      |  /|
 *      | / |
 *      |/  |
 *      3---2
 *
 *  colors and/or texs can be null. The resulting triangles should be passed to drawMesh(...).
 */
    void drawQuad(const GPoint verts[4], const GColor colors[4], const GPoint texs[4], int level, const GPaint& paint) {
        if (colors == nullptr && texs == nullptr) {
            return;
        }
        int quadNum = (int)pow(level + 1, 2);
        int triNum = quadNum * 2;
        int cornerNum = (int)pow(level + 2, 2);
        GPoint corners[cornerNum];
        GColor color[cornerNum];
        GPoint texture[cornerNum];

        int x = 0;
        for (int i = 0; i <= level + 1; i++) {
            float v = (float) i / (level + 1);
            for (int j = 0; j <= level + 1; j++) {
                float u = (float) j / (level + 1);
                corners[x] = findPoint(verts, u, v);
                if (colors != nullptr) {
                    color[x] = findColor(colors, u, v);
                }
                if (texs != nullptr) {
                    texture[x] = findPoint(texs, u, v);
                }
                x++;
            }
        }

        int indices[quadNum * 6];
        int b = 0;
        int c = 0;

        for (int a = 0; a <= level; a++) {
            indices[b] = c;
            indices[b + 1] = c + 1;
            indices[b + 2] = c + level + 2;

            c += 1;
            b += 3;

            for (int d = 0; d < level; d++) {
                indices[b] = c;
                indices[b + 1] = c + level + 2;
                indices[b + 2] = c + level + 1;

                indices[b + 3] = c;
                indices[b + 4] = c + 1;
                indices[b + 5] = c + level + 2;

                c += 1; 
                b += 6;
            }

            indices[b] = c;
            indices[b + 1] = c + 2 + level;
            indices[b + 2] = c + level + 1;

            b += 3;
            c += 1;

        }

        if (texs == nullptr && colors == nullptr) {
            drawMesh(corners, nullptr, nullptr, triNum, indices, paint);
        }
        else if (texs == nullptr) {
            drawMesh(corners, color, nullptr, triNum, indices, paint);
        }
        else if (colors == nullptr) {
            drawMesh(corners, nullptr, texture, triNum, indices, paint);
        }
        else {
            drawMesh(corners, color, texture, triNum, indices, paint);
        }

    }

private:
    // Note: we store a copy of the bitmap
    const GBitmap fDevice;

    std::stack<GMatrix> stack;
    GMatrix matrix; // identity matrix

    void paintRow(int y, int leftX, int rightX, const GPaint& paint) {
        leftX = std::max(0, leftX);
        rightX = std::min(fDevice.width(), rightX);
        GPixel src = colorToPixel(paint.getColor().pinToUnit());

        blendProc blender = getProc(paint.getBlendMode(), src);
        bool srcover = (blender == kSrcOver);

        if (blender == kDst) {
            return;
        }

        if (paint.getShader() == nullptr) {
            if (rightX <= leftX) {
                return;
            }
            GPixel* addr = fDevice.getAddr(leftX, y);
           // for (int x = leftX; x < rightX; x++) {
                if (srcover) {
                    for (int x = leftX; x < rightX; x++) {
                        *addr = kSrcOver(src, *addr);
                        *addr++;
                    }
                }
                else if (blender == kSrc) {
                    for (int x = leftX; x < rightX; x++) {
                        *addr = kSrc(src, *addr);
                        *addr++;
                    }
                }
                else {
                    for (int x = leftX; x < rightX; x++) {
                        *addr = blender(src, *addr);
                        *addr++;
                    }
                }
               // *addr++;
            //}
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

            //for (int x = leftX; x < rightX; x++) {
                if (srcover) {
                    for (int x = leftX; x < rightX; x++) {
                        *addr = kSrcOver(row[x - leftX], *addr);
                        *addr++;
                    }
                    
                }
                else if (blender == kSrc) {
                    for (int x = leftX; x < rightX; x++) {
                        *addr = kSrc(row[x - leftX], *addr);
                        *addr++;
                    }

                }
                else {
                    for (int x = leftX; x < rightX; x++) {
                        *addr = blender(row[x - leftX], *addr);
                        *addr++;
                    }

                }
               // *addr++;
            //}
        }
    }

    void blit(int x0, int x1, int y, const GPaint& paint) {
        GShader* shader = paint.getShader();
        GPixel src = colorToPixel(paint.getColor());

        blendProc blender = getProc(paint.getBlendMode(), src);
        if (blender == kDst) {
           return;
        }
        //bool srcover = (blender == kSrcOver);

        if (shader != nullptr) {
            int count = x1 - x0;
            assert(x0 >= 0 && count >= 0); 
            GPixel row[count];
            shader->shadeRow(x0, y, count, row);
            
            for (int x = x0; x < x1; x++) {
                GPixel* addr = fDevice.getAddr(x, y);
                //if (srcover) {
                //    *addr = kSrcOver(row[x - x0], *addr);
                //}
                //else {
                    *addr = blender(row[x - x0], *addr);
                //}
            }
        }
        else {
            for (int x = x0; x < x1; x++) {
                GPixel* addr = fDevice.getAddr(x, y);
                //if (srcover) {
                //    *addr = kSrcOver(src, *addr);
                //}
                //else {
                    *addr = blender(src, *addr);
                //}
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

    int numQuadSegments(GPoint pt[3]) { // sqrt(|d| / t)
        GPoint d = (-1 * pt[0] + 2 * pt[1] - pt[2]) * 0.25; // d = absolute by (-1*(a - 2b + c)) / 4
        float mag = sqrt(d.fX * d.fX + d.fY * d.fY); // magnitude of d
        return ceil(sqrt(4 * mag));
    }

    int numCubicSegments(GPoint pt[4]) {
        GPoint abc = 1 * pt[0] - 2 * pt[1] + pt[2]; // a - 2b + c
        GPoint bcd = 1 * pt[1] - 2 * pt[2] + pt[3]; // b - 2c + d
        GPoint d = { std::max(std::abs(abc.fX), std::abs(bcd.fX)), std::max(std::abs(abc.fY), std::abs(bcd.fY)) }; // create d with max [abc.x, bcd.x] and max [abc.y, bcd.y]
        float mag = sqrt(d.fX * d.fX + d.fY * d.fY); // magntiude of d
        return ceil(sqrt(3 * mag));
    }

    GPoint getQuadPt(GPoint src[3], float t) { // gets t as stored in dst[2] in chop quad
        return (1 - t)* (1 - t)* src[0] + 2 * t * (1 - t) * src[1] + t * t * src[2];
    }

    GPoint getCubicPt(GPoint src[4], float t) { // gets t as stored in dst[3] in chop cubic
        return (1 - t) * (1 - t) * (1 - t) * src[0] + 3 * t * (1 - t) * (1 - t) * src[1] + 3 * t * t * (1 - t) * src[2] + t * t * t * src[3];
    }

    GColor findColor(const GColor colors[4], float u, float v) {
        return (1.0f - u) * (1.0f - v) * colors[0] + u * (1.0f - v) * colors[1] + u * (v * colors[2]) + v * (1.0f - u) * colors[3];
    }

    GPoint findPoint(const GPoint points[4], float u, float v) {
        return (1.0f - u) * (1.0f - v) * points[0] + u * (1.0f - v) * points[1] + u * (v * points[2]) + v * (1.0f - u) * points[3];
    }
};

std::unique_ptr<GCanvas> GCreateCanvas(const GBitmap& device) {
    return std::unique_ptr<GCanvas>(new my_canvas(device));
}

static void draw_tri(GCanvas* canvas) {
    const GPoint pts[] = {
        { 10, 10 }, { 400, 100 }, { 250, 400 },
    };
    const GColor clr[] = {
        { 0, 1, 0, 1 }, { 0, 0, 1, 1 }, { 1, 0, 0, 1 },
    };
    const int indices[] = {
        0, 1, 2,
    };

    canvas->translate(25, 25);
    canvas->rotate(45);
    canvas->translate(-25, -25);
    canvas->drawMesh(pts, clr, nullptr, 1, indices, GPaint());
}

static void draw_tri2(GCanvas* canvas) {
    const GPoint pts[] = {
        { -10, -10 }, { 200, 100 }, { 200, 150 },
    };
    const GColor clr[] = {
        { 0, 1, 0, 1 }, { 1, 0, 0, 1 }, { 0, 0, 1, 1 },
    };
    const int indices[] = {
        0, 1, 2,
    };
    canvas->translate(-10, -50);
    canvas->drawMesh(pts, clr, nullptr, 1, indices, GPaint());
}

std::string GDrawSomething(GCanvas* canvas, GISize dim) {

    draw_tri(canvas);
    draw_tri2(canvas);

    return "tri-ombres";
}
