/**
 *  Copyright 2015 Mike Reed
 */

#include "GCanvas.h"
#include "GBitmap.h"
#include "GColor.h"
#include "GPoint.h"
#include "GRect.h"
#include "tests.h"

#include "tests_pa1.cpp"
#include "tests_pa2.cpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

const GTestRec gTestRecs[] = {
    { test_clear,       "clear"         },
    { test_rect_nodraw, "rect_nodraw"   },

    { test_poly_nodraw, "poly_nodraw"   },

    { nullptr, nullptr },
};

bool gTestSuite_Verbose;
bool gTestSuite_CrashOnFailure;
