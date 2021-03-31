#ifndef __BLURMENU_H
#define __BLURMENU_H

#include <cairo.h>
#include <cairo-xlib.h>
#include <pango/pangocairo.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/extensions/Xrandr.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

struct box {
	int x; int y; int w; int h;
};

struct xwindow {
	Display *dpy;
	Window win;
	Window root;
	GC gc;		/* graphics context */
	XIC xic;	/* input context */
	XIM xim;	/* input method */
	cairo_t *cr;	/* cairo drawing context */
	Pixmap canvas;	/* X drawable pixmap */

	cairo_surface_t *blurred_scrot;

	/* output coordinates of the crt the window is on */
	struct box screen_geo;

	/* output coordinates of the window */
	struct box window_geo;
};

cairo_surface_t *surface_from_ximage(XImage *ximg, int width, int height);

void select_area(int * cordx, int * cordy, int * cordw, int * cordh);

#endif /* __BLURMENU_H */
