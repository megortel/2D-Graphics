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


/* Edges */
struct Edge {
    float m, currX, b;
    int bottom, top, wind;

    void set(float slope, float x, float bvalue, int t, int bott, int winding) {
        m = slope;
        currX = x;
        b = bvalue;
        top = t;
        bottom = bott;
        wind = winding;
    }

    void set(GPoint left, GPoint right, int w) {
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
        wind = w;
    }

    bool init(GPoint p0, GPoint p1, int w) { // init returns bool, set returns nothing
        // Ensure p0.y <= p1.y
        if (p0.y() > p1.y()) {
            std::swap(p0, p1);
        }

        this->top = GRoundToInt(p0.y());
        this->bottom = GRoundToInt(p1.y());
        this->wind = w;

        if (top == bottom) {
            return false;
        }

        this->m = (p1.x() - p0.x()) / (bottom - top);

        float dx = this->m * (this->top - p0.y() + 0.5f);
        this->currX = p0.x() + dx;

        return true;
    }

    int calculateX(int y) {
        return GRoundToInt(m * (y + 0.5) + b);
    }

    bool isValid(int y) {
        if (y >= top && y < bottom) {
            return true;
        }
        return false;
    }
};

Edge* createEdge(GPoint left, GPoint right, bool w) {
    assert(GRoundToInt(left.y()) != GRoundToInt(right.y()));
    if (left.y() > right.y()) {
        std::swap(left, right);
    }
    Edge* e = new Edge();
    int winding;
    if (w) {
        winding = 1;
    }
    else {
        winding = -1;
    }
    e->set(left, right, winding);

    return e;
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

bool edgeComparisonX(Edge e1, Edge e2) {
    return e1.currX < e2.currX;
}