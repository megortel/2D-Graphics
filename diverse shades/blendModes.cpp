#include "GPixel.h"
#include "GPaint.h"
#include "blendModes.h"



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


/* Blend Modes */
GPixel kClear(const GPixel src, const GPixel dst) {
    return GPixel_PackARGB(0, 0, 0, 0);
}


GPixel kDst(const GPixel src, const GPixel dst) {
    return dst;
}


GPixel kDstOver(const GPixel src, const GPixel dst) { // D + (1- Da)*S
    int sa = GPixel_GetA(src);
    int sr = GPixel_GetR(src);
    int sg = GPixel_GetG(src);
    int sb = GPixel_GetB(src);

    int da = GPixel_GetA(dst);
    int dr = GPixel_GetR(dst);
    int dg = GPixel_GetG(dst);
    int db = GPixel_GetB(dst);

    int a = da + dividePixel((255 - da) * sa);
    int r = dr + dividePixel((255 - da) * sr);
    int g = dg + dividePixel((255 - da) * sg);
    int b = db + dividePixel((255 - da) * sb);

    return GPixel_PackARGB(a, r, g, b);
}

GPixel kSrcIn(const GPixel src, const GPixel dst) { //Da*S
    int sa = GPixel_GetA(src);
    int sr = GPixel_GetR(src);
    int sg = GPixel_GetG(src);
    int sb = GPixel_GetB(src);

    int da = GPixel_GetA(dst);

    int a = dividePixel(da * sa);
    int r = dividePixel(da * sr);
    int g = dividePixel(da * sg);
    int b = dividePixel(da * sb);

    return GPixel_PackARGB(a, r, g, b);
}

GPixel kDstIn(const GPixel src, const GPixel dst) { // Sa*D
    int sa = GPixel_GetA(src);

    int da = GPixel_GetA(dst);
    int dr = GPixel_GetR(dst);
    int dg = GPixel_GetG(dst);
    int db = GPixel_GetB(dst);


    int a = dividePixel(sa * da);
    int r = dividePixel(sa * dr);
    int g = dividePixel(sa * dg);
    int b = dividePixel(sa * db);

    return GPixel_PackARGB(a, r, g, b);
}

GPixel kSrcOut(const GPixel src, const GPixel dst) { //(1-Da)*S
    int sa = GPixel_GetA(src);
    int sr = GPixel_GetR(src);
    int sg = GPixel_GetG(src);
    int sb = GPixel_GetB(src);

    int da = GPixel_GetA(dst);

    int a = dividePixel((255 - da) * sa);
    int r = dividePixel((255 - da) * sr);
    int g = dividePixel((255 - da) * sg);
    int b = dividePixel((255 - da) * sb);

    return GPixel_PackARGB(a, r, g, b);
}

GPixel kDstOut(const GPixel src, const GPixel dst) { //(1-Sa)*D
    int sa = GPixel_GetA(src);

    int da = GPixel_GetA(dst);
    int dr = GPixel_GetR(dst);
    int dg = GPixel_GetG(dst);
    int db = GPixel_GetB(dst);

    int a = dividePixel((255 - sa) * da);
    int r = dividePixel((255 - sa) * dr);
    int g = dividePixel((255 - sa) * dg);
    int b = dividePixel((255 - sa) * db);

    return GPixel_PackARGB(a, r, g, b);
}

GPixel kSrcATop(const GPixel src, const GPixel dst) { //Da*S + (1-Sa)*D
    int sa = GPixel_GetA(src);
    int sr = GPixel_GetR(src);
    int sg = GPixel_GetG(src);
    int sb = GPixel_GetB(src);

    int da = GPixel_GetA(dst);
    int dr = GPixel_GetR(dst);
    int dg = GPixel_GetG(dst);
    int db = GPixel_GetB(dst);

    int a = dividePixel(da * sa + (255 - sa) * da);
    int r = dividePixel(da * sr + (255 - sa) * dr);
    int g = dividePixel(da * sg + (255 - sa) * dg);
    int b = dividePixel(da * sb + (255 - sa) * db);

    return GPixel_PackARGB(a, r, g, b);
}

GPixel kDstATop(const GPixel src, const GPixel dst) { // Sa*D + (1-Da)*S
    int sa = GPixel_GetA(src);
    int sr = GPixel_GetR(src);
    int sg = GPixel_GetG(src);
    int sb = GPixel_GetB(src);

    int da = GPixel_GetA(dst);
    int dr = GPixel_GetR(dst);
    int dg = GPixel_GetG(dst);
    int db = GPixel_GetB(dst);


    int a = dividePixel(sa * da + (255 - da) * sa);
    int r = dividePixel(sa * dr + (255 - da) * sr);
    int g = dividePixel(sa * dg + (255 - da) * sg);
    int b = dividePixel(sa * db + (255 - da) * sb);

    return GPixel_PackARGB(a, r, g, b);
}

GPixel kXor(const GPixel src, const GPixel dst) { //(1 - Sa)*D + (1 - Da)*S
    int sa = GPixel_GetA(src);
    int sr = GPixel_GetR(src);
    int sg = GPixel_GetG(src);
    int sb = GPixel_GetB(src);

    int da = GPixel_GetA(dst);
    int dr = GPixel_GetR(dst);
    int dg = GPixel_GetG(dst);
    int db = GPixel_GetB(dst);

    if (sa == 255 || da == 0) {
        int a = dividePixel((255 - da) * sa);
        int r = dividePixel((255 - da) * sr);
        int g = dividePixel((255 - da) * sg);
        int b = dividePixel((255 - da) * sb);
        return GPixel_PackARGB(a, r, g, b);
    }

    else if (da == 255 || sa == 0) {
        int a = dividePixel((255 - sa) * da);
        int r = dividePixel((255 - sa) * dr);
        int g = dividePixel((255 - sa) * dg);
        int b = dividePixel((255 - sa) * db);
        return GPixel_PackARGB(a, r, g, b);
    }

    else {
        int a = dividePixel((255 - sa) * da + (255 - da) * sa);
        int r = dividePixel((255 - sa) * dr + (255 - da) * sr);
        int g = dividePixel((255 - sa) * dg + (255 - da) * sg);
        int b = dividePixel((255 - sa) * db + (255 - da) * sb);
        return GPixel_PackARGB(a, r, g, b);
    }
}


blendProc getProc(const GBlendMode mode) {
    return blendtypes[static_cast<int>(mode)];
}

blendProc getProc(const GBlendMode mode, const GPixel src) {
    int sa = GPixel_GetA(src);
    //if (sa == 255 && mode == GBlendMode::kSrcOver) { //go through other blendmodes and optimize here, focus on src
    //    return kSrc;
    //}

    if (sa == 0 && (mode == GBlendMode::kSrcIn || mode == GBlendMode::kDstIn || mode == GBlendMode::kSrcOut || mode == GBlendMode::kDstATop)) {
        return kClear;
    }

    return getProc(mode);
}
