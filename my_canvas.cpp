#include <iostream>
#include <iostream>

#include "GCanvas.h"
#include "GRect.h"
#include "GColor.h"
#include "GBitmap.h"
#include "GPixel.h"

int dividePixel(int value) {
    return (((value + 128) * 257) >> 16);
}

GPixel srcover(const GPixel& src, const GPixel& dst) {
    int sa = GPixel_GetA(src);
    int sr = GPixel_GetR(src);
    int sg = GPixel_GetG(src);
    int sb = GPixel_GetB(src);

    int da = GPixel_GetA(dst);
    int dr = GPixel_GetR(dst);
    int dg = GPixel_GetG(dst);
    int db = GPixel_GetB(dst);

    int a = sa + dividePixel((255 - sa) * da);
    int r = sr + dividePixel((255 - sa) * dr);
    int g = sg + dividePixel((255 - sa) * dg);
    int b = sb + dividePixel((255 - sa) * db);

    return GPixel_PackARGB(a, r, g, b);
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

class my_canvas : public GCanvas {
public:
    my_canvas(const GBitmap& device) : fDevice(device) {}

    void clear(const GColor& color) override {
        GPixel p = color2pixel(color);

        int width = fDevice.width();
        int height = fDevice.height();
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                GPixel* addy = fDevice.getAddr(x, y);
                *addy = p;
            }
        }
    }

    void drawRect(const GRect& rect, const GColor& color) override {
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
        GPixel src = color2pixel(color);
        /* opague and transparent colors test */
        if (GPixel_GetA(src) == 255) {
            for (int y = T; y < B; ++y) {
                for (int x = L; x < R; ++x) {
                    GPixel* p = fDevice.getAddr(x, y);
                    *p = src;
                }
            }
        }
        else if (GPixel_GetA(src) == 0) {
            /* do nothing*/
        }
        else {
            for (int y = T; y < B; ++y) {
                for (int x = L; x < R; ++x) {
                    GPixel* p = fDevice.getAddr(x, y);
                    GPixel newp = srcover(src, *p);
                    *p = newp;
                }

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

std::string GDrawSomething(GCanvas* canvas, GISize dim) {
    GColor srccolor;
    srccolor.a = 0.9;
    srccolor.r = 0.59;
    srccolor.g = 0.73;
    srccolor.b = 0.86;

    canvas->clear(srccolor);
   
    GColor dstcolor; 
    dstcolor.a = 0.5;
    dstcolor.r = 0.0;
    dstcolor.g = 0.54;
    dstcolor.b = 0.54;

    GColor dstcolor2;
    dstcolor2.a = 0.4;
    dstcolor2.r = 0.15;
    dstcolor2.g = 0.2;
    dstcolor2.b = 0.6;

    GRect rect = GRect::XYWH(0, 0, 50, 50);
    GRect rect2 = GRect::XYWH(0, 0, 100, 100);
    GRect rect3 = GRect::XYWH(0, 0, 150, 150);
    GRect rect4 = GRect::XYWH(0, 0, 200, 200);
    GRect rect5 = GRect::XYWH(0, 0, 250, 250);

    canvas->drawRect(rect, dstcolor);
    canvas->drawRect(rect2, dstcolor2);
    canvas->drawRect(rect3, dstcolor);
    canvas->drawRect(rect4, dstcolor2);
    canvas->drawRect(rect5, dstcolor);

    return "rectangle mania";
}