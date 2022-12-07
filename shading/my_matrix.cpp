#include "GMath.h"

#include "GMatrix.h"
#include "GPoint.h"

GMatrix::GMatrix() {
    fMat[0] = 1;
    fMat[1] = 0;
    fMat[2] = 0;
    fMat[3] = 0;
    fMat[4] = 1;
    fMat[5] = 0;
}

GMatrix GMatrix::Translate(float tx, float ty) {
    return GMatrix(1, 0, tx, 0, 1, ty);
}

GMatrix GMatrix::Scale(float sx, float sy) {
    return GMatrix(sx, 0, 0, 0, sy, 0);
}

GMatrix GMatrix::Rotate(float radians) {
    return GMatrix(cos(radians), -1 * sin(radians), 0, sin(radians), cos(radians), 0);
}

/**
 *  Return the product of two matrices: a * b
 */
GMatrix GMatrix::Concat(const GMatrix& a, const GMatrix& b) {
    float aVal = a[0] * b[0] + a[1] * b[3];
    float bVal = a[0] * b[1] + a[1] * b[4];
    float cVal = a[0] * b[2] + a[1] * b[5] + a[2];
    float dVal = a[3] * b[0] + a[4] * b[3];
    float eVal = a[3] * b[1] + a[4] * b[4];
    float fVal = a[3] * b[2] + a[4] * b[5] + a[5];
    return GMatrix(aVal, bVal, cVal, dVal, eVal, fVal);
}

/*
 *  Compute the inverse of this matrix, and store it in the "inverse" parameter, being
 *  careful to handle the case where 'inverse' might alias this matrix.
 *
 *  If this matrix is invertible, return true. If not, return false, and ignore the
 *  'inverse' parameter.
 */
bool GMatrix::invert(GMatrix* inverse) const {
    float aVal = fMat[0];
    float bVal = fMat[1];
    float cVal = fMat[2];
    float dVal = fMat[3];
    float eVal = fMat[4];
    float fVal = fMat[5];

    float determinant = (aVal * eVal - bVal * dVal);
    if (determinant == 0) {
        return false;
    }

    float divide = 1 / determinant;

    *inverse = GMatrix(eVal * divide, -1 * bVal * divide, (bVal * fVal - cVal * eVal) * divide, -1 * dVal * divide, aVal * divide, (cVal * dVal - aVal * fVal) * divide);
    return true;
}

/**
 *  Transform the set of points in src, storing the resulting points in dst, by applying this
 *  matrix. It is the caller's responsibility to allocate dst to be at least as large as src.
 *
 *  [ a  b  c ] [ x ]     x' = ax + by + c
 *  [ d  e  f ] [ y ]     y' = dx + ey + f
 *  [ 0  0  1 ] [ 1 ]
 *
 *  Note: It is legal for src and dst to point to the same memory (however, they may not
 *  partially overlap). Thus the following is supported.
 *
 *  GPoint pts[] = { ... };
 *  matrix.mapPoints(pts, pts, count);
 */
void GMatrix::mapPoints(GPoint dst[], const GPoint src[], int count) const {
    for (int i = 0; i < count; i++) {

        float x0 = src[i].x();
        float y0 = src[i].y();

        float x = (fMat[0] * x0) + (fMat[1] * y0) + fMat[2];
        float y = (fMat[3] * x0) + (fMat[4] * y0) + fMat[5];

        dst[i].fX = x;
        dst[i].fY = y;
    }
}