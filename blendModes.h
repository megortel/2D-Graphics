#ifndef blendModes_DEFINED
#define blendModes_DEFINED

#include "GPixel.h"
#include "GPaint.h"
#include "GBlendMode.h"

typedef GPixel(*blendProc)(GPixel, GPixel);

static inline int dividePixel(int value) {
    return (((value + 128) * 257) >> 16);
}

static inline GPixel kSrc(const GPixel src, const GPixel dst) {
    return src;
}

static inline GPixel kSrcOver(const GPixel src, const GPixel dst) { // S + (1-Sa)*D
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

GPixel kClear(const GPixel src, const GPixel dst);
GPixel kDst(const GPixel src, const GPixel dst);
GPixel kDstOver(const GPixel src, const GPixel dst);
GPixel kSrcIn(const GPixel src, const GPixel dst);
GPixel kDstIn(const GPixel src, const GPixel dst);
GPixel kSrcOut(const GPixel src, const GPixel dst);
GPixel kDstOut(const GPixel src, const GPixel dst);
GPixel kSrcATop(const GPixel src, const GPixel dst);
GPixel kDstATop(const GPixel src, const GPixel dst);
GPixel kXor(const GPixel src, const GPixel dst);


/* Array to hold enum values of blendmodes*/
const blendProc blendtypes[] = {
    kClear,
    kSrc,
    kDst,
    kSrcOver,
    kDstOver,
    kSrcIn,
    kDstIn,
    kSrcOut,
    kDstOut,
    kSrcATop,
    kDstATop,
    kXor,
};

/* Get the blendmode */
blendProc getProc(const GBlendMode mode);

/* get blendmode with pixel parm */
blendProc getProc(const GBlendMode mode, const GPixel src);


#endif
