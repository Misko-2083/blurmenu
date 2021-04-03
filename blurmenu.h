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

	/* root window coordinates of the crt the menu window is on */
	struct box screen_geo;

	/* root window coordinates of the menu window */
	struct box window_geo;
};

struct menuitem {
	char *name;
	struct box box;
	cairo_surface_t *texture;
};

struct menu {
	struct box box;

	/* vector containing menu items */
	struct menuitem *items;
	int nr_items, alloc_items;

	/* pointer to currently selected item */
	struct menuitem *selected;
};

void items_init(struct menu *menu);
void items_finish(struct menu *menu);
void items_handle_key(struct menu *menu, int step);

void die(const char *fmt, ...);

cairo_surface_t *texture_create(struct box *box, const char *text, double *bg, double *fg);
cairo_surface_t *surface_from_ximage(XImage *ximg, int width, int height);

void select_area(int * cordx, int * cordy, int * cordw, int * cordh);

/* get the geometry of the crt where the pointer is */
void x11_crt_geometry(struct xwindow *ctx, struct box *box);

/* create a transparent window with no border */
void x11_create_window(struct xwindow *ctx, struct box *box, XVisualInfo *vinfo);

#endif /* __BLURMENU_H */
