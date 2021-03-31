#ifndef __BLURMENU_H
#define __BLURMENU_H

#include <cairo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

struct box {
	int x; int y; int w; int h;
};

cairo_surface_t *surface_from_ximage(XImage *ximg, int width, int height);

void select_area(int * cordx, int * cordy, int * cordw, int * cordh);

#endif /* __BLURMENU_H */
