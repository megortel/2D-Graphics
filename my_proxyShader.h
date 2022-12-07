#ifndef my_proxyShader_DEFINED
#define my_proxyShader_DEFINED

#include "GBitmap.h"
#include "GMatrix.h"
#include "GShader.h"

class my_proxyShader : public GShader {
public:
    my_proxyShader(GShader* shader, const GMatrix& trans) : frs(shader), transform(trans) {
    }

    // Return true iff all of the GPixels that may be returned by this shader will be opaque.
    bool isOpaque() override {
        return frs->isOpaque();
    }

    // The draw calls in GCanvas must call this with the CTM before any calls to shadeSpan().
    bool setContext(const GMatrix& ctm) override {
        return frs->setContext(ctm * transform);
    }

    /**
     *  Given a row of pixels in device space [x, y] ... [x + count - 1, y], return the
     *  corresponding src pixels in row[0...count - 1]. The caller must ensure that row[]
     *  can hold at least [count] entries.
     */
    void shadeRow(int x, int y, int count, GPixel row[]) override {
        frs->shadeRow(x, y, count, row);
    }


private:
    GShader* frs;
    GMatrix transform;
};

/**
 *  Return a subclass of GShader that draws the specified bitmap and the local matrix.
 *  Returns null if the either parameter is invalid.
 */
std::unique_ptr<GShader> GCreateProxyShader(GShader* shader, const GMatrix& trans) {
    return std::unique_ptr<GShader>(new my_proxyShader(shader, trans));
}

#endif
