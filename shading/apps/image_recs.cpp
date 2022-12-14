/**
 *  Copyright 2015 Mike Reed
 */

#include "image.h"

#include "image_pa1.cpp"
#include "image_pa2.cpp"
#include "image_pa3.cpp"
#include "image_pa4.cpp"

const GDrawRec gDrawRecs[] = {
    { draw_solid_ramp,  256, 7*28,  "solid_ramp",   1   },
    { draw_graphs,      256, 256,   "rect_graphs",  1   },
    { draw_blend_black, 200, 200,   "blend_black",  1   },

    { draw_poly,        512, 512,   "poly",         2 },
    { draw_poly_center, 256, 256,   "poly_center",  2 },
    { draw_blendmodes,  450, 340,   "blendmodes",   2 },

    { draw_checker,     300, 300,   "checkers",     3 },
    { draw_poly_rotate, 300, 300,   "color_clock",  3 },
    { draw_bitmaps_hole,300, 300,   "bitmap_hole",  3 },
    { draw_clock_bm,    480, 480,   "spock_clock",  3 },
    { draw_bm_blendmodes,  450, 340,   "blendmodes2",   3 },

    { stars,            512, 512,   "stars",        4 },
    { draw_lion,        512, 512,   "lion",         4 },
    { draw_lion_head,   512, 512,   "lion_head",    4 },
    { draw_grad,        250, 200,   "grad",         4 },
    { draw_gradient_blendmodes, 450, 340, "gradient_blendmodes", 4 },
    { draw_graphs2,     256, 256,   "path_graphs",  4  },

    { nullptr, 0, 0, nullptr },
};
