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
#include "tests_pa3.cpp"
#include "tests_pa4.cpp"

const GTestRec gTestRecs[] = {
    { test_clear,       "clear"         },
    { test_rect_nodraw, "rect_nodraw"   },

    { test_poly_nodraw, "poly_nodraw"   },

    { test_matrix,      "matrix_setters"    },
    { test_matrix_inv,  "matrix_inv"        },
    { test_matrix_map,  "matrix_map"        },

    { test_path,        "path",             },
    { test_path_rect,   "path_rect",        },
    { test_path_poly,   "test_path_poly",   },
    { test_path_transform, "path_transform" },

    { nullptr, nullptr },
};

bool gTestSuite_Verbose;
bool gTestSuite_CrashOnFailure;
