#include "GBitmap.h"
#include "GMatrix.h"
#include "GShader.h"

class my_shader : public GShader {
public:
    my_shader(const GBitmap& bitmap, const GMatrix& matrix) : fSourceBitmap(bitmap), fLocalMatrix(matrix) {
        rctm = GMatrix();
    }

    // Return true iff all of the GPixels that may be returned by this shader will be opaque.
    bool isOpaque() override {
        return fSourceBitmap.isOpaque();
    }

    // The draw calls in GCanvas must call this with the CTM before any calls to shadeSpan().
    bool setContext(const GMatrix& ctm) override {
        return (ctm * fLocalMatrix).invert(&rctm);
    }

    /**
     *  Given a row of pixels in device space [x, y] ... [x + count - 1, y], return the
     *  corresponding src pixels in row[0...count - 1]. The caller must ensure that row[]
     *  can hold at least [count] entries.
     */
    void shadeRow(int x, int y, int count, GPixel row[]) override {
        for (int i = 0; i < count; i++) {
            GPoint point = { x + 0.5 + i, y + 0.5 };
            GPoint lm = rctm * point;
            int srcX = lm.fX;
            int srcY = lm.fY;

            // clamp x and y, min of width/height and x, y and max of 0
            srcX = std::max(0, std::min(fSourceBitmap.width() - 1, srcX));
            srcY = std::max(0, std::min(fSourceBitmap.height() - 1, srcY));

            row[i] = *fSourceBitmap.getAddr(srcX, srcY);
        }
    }

private:
    GBitmap fSourceBitmap;
    GMatrix fLocalMatrix;
    GMatrix rctm;
};

/**
 *  Return a subclass of GShader that draws the specified bitmap and the local matrix.
 *  Returns null if the either parameter is invalid.
 */
std::unique_ptr<GShader> GCreateBitmapShader(const GBitmap& mbitmap, const GMatrix& localMatrix) {

    return std::unique_ptr<GShader>(new my_shader(mbitmap, localMatrix));
}
