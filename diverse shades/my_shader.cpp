#include "GBitmap.h"
#include "GMatrix.h"
#include "GShader.h"

class my_shader : public GShader {
public:
    my_shader(const GBitmap& bitmap, const GMatrix& matrix, GShader::TileMode tileMode) : fSourceBitmap(bitmap), fLocalMatrix(matrix), tile(tileMode) {
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

            switch (tile) {
                case TileMode::kClamp: {
                    // clamp x and y, min of width/height and x, y and max of 0
                    srcX = std::max(0, std::min(fSourceBitmap.width() - 1, srcX));
                    srcY = std::max(0, std::min(fSourceBitmap.height() - 1, srcY));
                    break;
                }
                case TileMode::kRepeat: {
                    // repeat x and y
                    srcX = repeat(lm.fX, fSourceBitmap.width());
                    srcY = repeat(lm.fY, fSourceBitmap.height());
                    break;
                }
                case TileMode::kMirror: {
                    //mirror then clamp
                    srcX = std::max(0, std::min(fSourceBitmap.width() - 1, mirror(lm.fX, fSourceBitmap.width())));
                    srcY = std::max(0, std::min(fSourceBitmap.height() - 1, mirror(lm.fY, fSourceBitmap.height())));
                    break;
                }
            }
            row[i] = *fSourceBitmap.getAddr(srcX, srcY);
        }
    }

    int repeat(float val, int canvas) {
        while (val < 0) {
            val += canvas;
        }
        while (val >= canvas) {
            val -= canvas;
        }
        return GFloorToInt(val);
    }

    int mirror(float val, int canvas) {
        if(val < 0) {
            val *= -1;
        }
        float value = val / canvas;
        int floored = GFloorToInt(value);
        int x = GFloorToInt(val);
        if (floored % 2 == 0) {
            return x % canvas;
        }
        else {
            return canvas - (x % canvas);
        }
    }

private:
    GBitmap fSourceBitmap;
    GMatrix fLocalMatrix;
    GMatrix rctm;
    GShader::TileMode tile;
};

/**
 *  Return a subclass of GShader that draws the specified bitmap and the local matrix.
 *  Returns null if the either parameter is invalid.
 */
std::unique_ptr<GShader> GCreateBitmapShader(const GBitmap& mbitmap, const GMatrix& localMatrix, GShader::TileMode tile) {

    return std::unique_ptr<GShader>(new my_shader(mbitmap, localMatrix, tile));
}
