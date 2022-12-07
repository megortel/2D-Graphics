#ifndef blendModes_DEFINED
#define blendModes_DEFINED

#include "GPixel.h"
#include "GPaint.h"
#include "GBlendMode.h"

typedef GPixel(*blendProc)(GPixel, GPixel);
 
GPixel kClear(const GPixel src, const GPixel dst);
GPixel kSrc(const GPixel src, const GPixel dst);
GPixel kDst(const GPixel src, const GPixel dst);
GPixel kSrcOver(const GPixel src, const GPixel dst);
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
