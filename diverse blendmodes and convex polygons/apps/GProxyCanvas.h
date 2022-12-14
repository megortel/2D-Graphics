/*
 *  Copyright 2017 Mike Reed
 */

#ifndef GProxyCanvas_DEFINED
#define GProxyCanvas_DEFINED

#include "GCanvas.h"

class GProxyCanvas : public GCanvas {
public:
    GProxyCanvas(GCanvas* proxy) : fProxy(proxy) {}

    bool virtual allowDraw() { return true; }

    void drawPaint(const GPaint& p) override {
        if (this->allowDraw()) {
            fProxy->drawPaint(p);
        }
    }

    void drawRect(const GRect& r, const GPaint& p) override {
        if (this->allowDraw()) {
            fProxy->drawRect(r, p);
        }
    }

    void drawConvexPolygon(const GPoint pts[], int count, const GPaint& p) override {
        if (this->allowDraw()) {
            fProxy->drawConvexPolygon(pts, count, p);
        }
    }

private:
    GCanvas* fProxy;
};

#endif
