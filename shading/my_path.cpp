#include <math.h>
#include "GMatrix.h"
#include "GPath.h"

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