#ifndef my_compositeShader_DEFINED
#define my_compositeShader_DEFINED

#include "GBitmap.h"
#include "GMatrix.h"
#include "GShader.h"
#include "blendModes.h"

class my_compositeShader : public GShader {
public:
    my_compositeShader(GShader* sh0, GShader* sh1) : s0(sh0), s1(sh1) {
    }

    // Return true iff all of the GPixels that may be returned by this shader will be opaque.
    bool isOpaque() override {
        return s0->isOpaque() && s1->isOpaque();
    }

    // The draw calls in GCanvas must call this with the CTM before any calls to shadeSpan().
    bool setContext(const GMatrix& ctm) override {
        return s0->setContext(ctm) && s1->setContext(ctm);
    }

    GPixel multPixels(GPixel p0, GPixel p1) {
        unsigned r, g, b, a;
        r = dividePixel(GPixel_GetR(p0) * GPixel_GetR(p1));
        g = dividePixel(GPixel_GetG(p0) * GPixel_GetG(p1));
        b = dividePixel(GPixel_GetB(p0) * GPixel_GetB(p1));
        a = dividePixel(GPixel_GetA(p0) * GPixel_GetA(p1));

        return GPixel_PackARGB(a, r, g, b);
    }

    /**
     *  Given a row of pixels in device space [x, y] ... [x + count - 1, y], return the
     *  corresponding src pixels in row[0...count - 1]. The caller must ensure that row[]
     *  can hold at least [count] entries.
     */
    void shadeRow(int x, int y, int count, GPixel row[]) override {
        GPixel p0[count];
        GPixel p1[count];

        s0->shadeRow(x, y, count, p0);
        s1->shadeRow(x, y, count, p1);
        for (int i = 0; i < count; i++) {
            row[i] = multPixels(p0[i], p1[i]);// need to add multPixels function
        }
    }


private:
   GShader* s0;
   GShader* s1;
};

/**
 *  Return a subclass of GShader that draws the specified bitmap and the local matrix.
 *  Returns null if the either parameter is invalid.
 */
std::unique_ptr<GShader> GCreateCompositeShader(GShader* sh0, GShader* sh1) {
    return std::unique_ptr<GShader>(new my_compositeShader(sh0, sh1));
}

#endif