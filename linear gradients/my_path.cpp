#include <math.h>
#include "GMatrix.h"
#include "GPath.h"
#include "GRect.h"

 GPath& GPath::addRect(const GRect& r, Direction dir) {
        float left = r.left();
        float right = r.right();
        float top = r.top();
        float bottom = r.bottom();

        moveTo(left, top);
        if (dir == Direction::kCW_Direction) {
            lineTo(right, top);
            lineTo(right, bottom);
            lineTo(left, bottom);
        }
        else {
            lineTo(left, bottom);
            lineTo(right, bottom);
            lineTo(right, top);
        }
        return *this;
    }

    GPath& GPath::addPolygon(const GPoint pts[], int count) {
        if (count < 2) {
            return *this;
        }
        moveTo(pts[0]);
        for (int i = 1; i < count; i++) {
            lineTo(pts[i]);
        }
        return *this;
    }

    GRect GPath::bounds() const {
        if (fPts.size() == 0) {
            return GRect::LTRB(0, 0, 0, 0);
        }
        if (fPts.size() == 1) {
            return GRect::XYWH(fPts.at(0).x(), fPts.at(0).y(), 0, 0);
        }

        float l = fPts.at(0).x();
        float r = fPts.at(0).x();
        float t = fPts.at(0).y();
        float b = fPts.at(0).y();

        for (int i = 1; i < fPts.size(); i++) {
            l = std::min(l, fPts.at(i).x());
            r = std::max(r, fPts.at(i).x());
            t = std::min(t, fPts.at(i).y());
            b = std::max(b, fPts.at(i).y());
        }
        return GRect::LTRB(l, t, r, b);
    }

    void GPath::transform(const GMatrix& m) {
        for (int i = 0; i < fPts.size(); i++) {
            fPts[i] = m * (fPts.at(i));
        }
    }

    /**
     *  Append a new contour respecting the Direction. The contour should be an approximate
     *  circle (8 quadratic curves will suffice) with the specified center and radius.
     *
     *  Returns a reference to this path.
     */
    GPath& GPath::addCircle(GPoint center, float radius, Direction direct) {
        float x = center.fX;
        float y = center.fY;

        float c = 0.55228474983079f;
        float dT = c * radius;
        GPath& path = moveTo({ x + radius, y });

        if (direct == Direction::kCW_Direction) {
            path.cubicTo({ x + radius, y - dT }, { x + dT, y - radius }, { x, y - radius });
            path.cubicTo({ x - dT, y - radius }, { x - radius, y - dT }, { x - radius, y });
            path.cubicTo({ x - radius, y + dT }, { x - dT, y + radius }, { x, y + radius });
            path.cubicTo({ x + dT, y + radius }, { x + radius, y + dT }, { x + radius, y });
        }
        else {
            path.cubicTo({ x + radius, y + dT }, { x + dT, y + radius }, { x, y + radius });
            path.cubicTo({ x - dT, y + radius }, { x - radius, y + dT }, { x - radius, y });
            path.cubicTo({ x - radius, y - dT }, { x - dT, y - radius }, { x, y - radius });
            path.cubicTo({ x + dT, y - radius }, { x + radius, y - dT }, { x + radius, y });
        }
        return path;
    }

    /**
     *  Given 0 < t < 1, subdivide the src[] quadratic bezier at t into two new quadratics in dst[]
    *  such that
    *  0...t is stored in dst[0..2]
     *  t...1 is stored in dst[2..4]
    */
    void GPath::ChopQuadAt(const GPoint src[3], GPoint dst[5], float t) {
        dst[0] = src[0];
        dst[1] = (1 - t) * src[0] + t * src[1];
        dst[2] = (1 - t) * (1 - t) * src[0] + 2 * t * (1 - t) * src[1] + t * t * src[2];
        dst[3] = (1 - t) * src[1] + t * src[2];
        dst[4] = src[2];
    }

    /**
     *  Given 0 < t < 1, subdivide the src[] cubic bezier at t into two new cubics in dst[]
     *  such that
     *  0...t is stored in dst[0..3]
     *  t...1 is stored in dst[3..6]
     */
    void GPath::ChopCubicAt(const GPoint src[4], GPoint dst[7], float t) {

        dst[0] = src[0];
        dst[1] = (1 - t) * src[0] + t * src[1];
        dst[2] = (1 - t) * ((1 - t) * src[0] + t * src[1]) + t * ((1 - t) * src[1] + t * src[2]);
        dst[3] = (1 - t) * (1 - t) * (1 - t) * src[0] + 3 * t * (1 - t) * (1 - t) * src[1] + 3 * t * t * (1 - t) * src[2] + t * t * t * src[3];
        dst[4] = (1 - t) * ((1 - t) * src[1] + t * src[2]) + t * ((1 - t) * src[2] + t * src[3]);
        dst[5] = (1 - t) * src[2] + t * src[3];
        dst[6] = src[3];

    }
