#ifndef my_triShader_DEFINED
#define my_triShader_DEFINED

#include "GBitmap.h"
#include "GMatrix.h"
#include "GShader.h"

class my_triShader : public GShader {
public:
    my_triShader(const GPoint pts[3], const GColor colors[3]){
        c0 = colors[0];
        c1 = colors[1];
        c2 = colors[2];
        p0 = pts[0];
        p1 = pts[1];
        p2 = pts[2];

        GPoint u = p1 - p0;
        GPoint v = p2 - p0;

        fInv = GMatrix();
        fMatrix = GMatrix(u.x(), v.x(), p0.x(), u.y(), v.y(), p0.y());

        dc1 = c1 - c0;
        dc2 = c2 - c0;
    }

    // Return true iff all of the GPixels that may be returned by this shader will be opaque.
    bool isOpaque() override {
        return (c0.a == 1.0 && c1.a == 1.0 && c2.a == 1.0);
    }

    int floatTOpixel(float value) {
        return floor(value * 255 + 0.5);
    }

    GPixel colorTOpixel(const GColor& c) {
        int a = floatTOpixel(c.a);
        int r = floatTOpixel(c.r * c.a);
        int g = floatTOpixel(c.g * c.a);
        int b = floatTOpixel(c.b * c.a);
        return GPixel_PackARGB(a, r, g, b);
    }


    // The draw calls in GCanvas must call this with the CTM before any calls to shadeSpan().
    bool setContext(const GMatrix& ctm) override {
        fMatrix.invert(&fInv);

        //fInv.setIdentity();


        return (ctm * fMatrix).invert(&fInv);
    }

    /**
     *  Given a row of pixels in device space [x, y] ... [x + count - 1, y], return the
     *  corresponding src pixels in row[0...count - 1]. The caller must ensure that row[]
     *  can hold at least [count] entries.
     */
    void shadeRow(int x, int y, int count, GPixel row[]) override {
        GPoint p = { x + 0.5, y + 0.5 };
        GPoint pt = fInv * p;

        GColor diffc1 = fInv[0] * dc1;
        GColor diffc2 = fInv[3] * dc2;

        dc = diffc1 + diffc2;

        GColor c = pt.x() * dc1 + pt.y() * dc2 + c0;
        for (int i = 0; i < count; i++) {
            row[i] = colorTOpixel(c);
            c += dc;
        }
    }


private:
    GMatrix fInv;
    GMatrix fMatrix;
    GColor c0;
    GColor c1;
    GColor c2;
    GPoint p0;
    GPoint p1;
    GPoint p2;
    GColor dc;
    GColor dc1;
    GColor dc2;
};

/**
 *  Return a subclass of GShader that draws the specified bitmap and the local matrix.
 *  Returns null if the either parameter is invalid.
 */
std::unique_ptr<GShader> GCreateTriColorShader(const GPoint pts[3], const GColor colors[3]) {
    return std::unique_ptr<GShader>(new my_triShader(pts, colors));
}

#endif