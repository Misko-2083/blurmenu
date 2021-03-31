/*
 * blurmenu - an attempt to create a menu with a blurred background
 *
 * Copyright (C) 2021 Милош Павловић
 * Copyright (C) 2021 Johan Malm
 */

#include <getopt.h>
#include "blurmenu.h"
#include "stackblur.h"

static Window win;
static XIC xic;
static GC gc;
static cairo_t *cr;
static Display *dpy;
static XIM xim;
static Window root;
static struct box screen_geo;
static struct box window_geo;

static const char blurmenu_usage[] =
"Usage: blurmenu [options]\n\n"
"  -h            Show help message and quit\n"
"  -r <radius>   Specify blur radius (1-50)\n"
"\n";

static void
usage(void)
{
	printf("%s", blurmenu_usage);
	exit(0);
}

static void die(const char *fmt, ...)
{
	va_list params;
	va_start(params, fmt);
	vfprintf(stderr, fmt, params);
	va_end(params);
	exit(1);
}

static bool is_point_on_crt(int x, int y, XRRCrtcInfo *ci)
{
	struct box b = {
		.x = (int)ci->x, .y = (int)ci->y,
		.w = (int)ci->width, .h = (int)ci->height,
	};
	if (x < b.x || x > b.x + b.w)
		return false;
	if (y < b.y || y > b.y + b.h)
		return false;
	return true;
}

/* return the geometry of the crt where the pointer is */
static void crt_geometry(struct box *box)
{
	int i, n, x, y, di;
	unsigned int du;
	Window dw;
	XRRScreenResources *sr;
	XRRCrtcInfo *ci = NULL;
	sr = XRRGetScreenResourcesCurrent(dpy, root);
	n = sr->ncrtc;
	XQueryPointer(dpy, root, &dw, &dw, &x, &y, &di, &di, &du);
	for (i = 0; i < n; i++) {
		if (ci)
			XRRFreeCrtcInfo(ci);
		ci = XRRGetCrtcInfo(dpy, sr, sr->crtcs[i]);
		if (!ci->noutput)
			continue;
		if (is_point_on_crt(x, y, ci)) {
			fprintf(stderr, "monitor=%d\n", i + 1);
			break;
		}
	}
	if (!ci)
		die("connection could be established to monitor");
	box->x = (int)ci->x;
	box->y = (int)ci->y;
	box->w = (int)ci->width;
	box->h = (int)ci->height;
	XRRFreeCrtcInfo(ci);
	XRRFreeScreenResources(sr);
}

/* create a transparent window with no border */
static void create_window(struct box *box, XVisualInfo *vinfo)
{
	XSetWindowAttributes swa;
	swa.override_redirect = True;
	swa.event_mask = ExposureMask | KeyPressMask |
			 VisibilityChangeMask | ButtonPressMask;
	swa.colormap = XCreateColormap(dpy, root, vinfo->visual, AllocNone);
	swa.background_pixel = 0;
	swa.border_pixel = 0;
	win = XCreateWindow(dpy, root, box->x, box->y, box->w, box->h, 0,
		vinfo->depth, CopyFromParent, vinfo->visual, CWOverrideRedirect |
		CWColormap | CWBackPixel | CWEventMask | CWBorderPixel, &swa);
	xic = XCreateIC(xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
		XNClientWindow, win, XNFocusWindow, win, NULL);
	gc = XCreateGC(dpy, win, 0, NULL);
	XStoreName(dpy, win, "blurmenu");
	XSetIconName(dpy, win, "blurmenu");

	XClassHint *classhint = XAllocClassHint();
	classhint->res_name = (char *)"blurmenu";
	classhint->res_class = (char *)"blurmenu";
	XSetClassHint(dpy, win, classhint);
	XFree(classhint);

	XDefineCursor(dpy, win, XCreateFontCursor(dpy, 68));
	XSync(dpy, False);
}

void render_image(cairo_surface_t *image, double x, double y, double alpha)
{
	cairo_save(cr);
	cairo_translate(cr, x, y);
	cairo_set_source_surface(cr, image, 0, 0);
	cairo_paint_with_alpha(cr, alpha);
	cairo_restore(cr);
}

void render_rectangle(struct box *box, double *rgba)
{
	double border_width = 1.0;
	double x = box->x + border_width / 2;
	double y = box->y + border_width / 2;
	double w = box->w - border_width;
	double h = box->h - border_width;

	cairo_set_source_rgba(cr, rgba[0], rgba[1], rgba[2], rgba[3]);
	cairo_set_line_width(cr, border_width);
	cairo_rectangle(cr, x, y, w, h);
//	cairo_fill(c);
	cairo_stroke(cr);
}

static void move(struct box *geo)
{
	XMoveResizeWindow(dpy, win, geo->x, geo->y, geo->w, geo->h);
}

static void handle_key_event(XKeyEvent *ev)
{
	char buf[32];
	KeySym ksym = NoSymbol;
	Status status;

	Xutf8LookupString(xic, ev, buf, sizeof(buf), &ksym, &status);
	if (status == XBufferOverflow)
		return;

	switch (ksym) {
	case XK_Escape:
		exit(0);
	case XK_Up:
		window_geo.y -= 5;
		move(&window_geo);
		break;
	case XK_Return:
	case XK_KP_Enter:
		exit(0);
	default:
		break;
	}
}

int main(int argc, char *argv[])
{
	int radius = 10;
	int c;
	while ((c = getopt(argc, argv, "hr:")) != -1) {
		switch (c) {
		case 'r':
			radius = atoi(optarg);
			break;
		case 'h':
		default:
			usage();
		}
	}
	if (optind < argc) {
		usage();
	}

	dpy = XOpenDisplay(NULL);
	if (!dpy)
		die("cannot open display");
	xim = XOpenIM(dpy, NULL, NULL, NULL);
	int screen = DefaultScreen(dpy);

	XVisualInfo vinfo;
	const char depths[] = { 32, 24, 8, 4, 2, 1, 0 };
	for (int i = 0; depths[i]; i++) {
		XMatchVisualInfo(dpy, screen, depths[i], TrueColor, &vinfo);
		if (vinfo.visual)
			break;
	}
	fprintf(stderr, "color depth=%d\n", vinfo.depth);

	root = RootWindow(dpy, screen);

	crt_geometry(&screen_geo);

	/* local co-ordinates */
	struct box menu = { .x = 0, .y = 0, .w = 600, .h = 400 };

	window_geo.x = screen_geo.x + (screen_geo.w - menu.w) / 2;
	window_geo.y = screen_geo.y + (screen_geo.h - menu.h) / 2;
	window_geo.w = menu.w;
	window_geo.h = menu.h;

	create_window(&window_geo, &vinfo);

	if (XGrabKeyboard(dpy, root, True, GrabModeAsync, GrabModeAsync,
			  CurrentTime) != GrabSuccess)
		die("XGrabKeyboard");
	if (XGrabPointer(dpy, root, False,
			 ButtonPressMask | ButtonReleaseMask |
			 PointerMotionMask | FocusChangeMask |
			 EnterWindowMask | LeaveWindowMask,
			 GrabModeAsync, GrabModeAsync, None, None,
			 CurrentTime) != GrabSuccess)
		die("XGrabPointer");

	Pixmap canvas = XCreatePixmap(dpy, root, menu.w, menu.h, vinfo.depth);
	cairo_surface_t *surf = cairo_xlib_surface_create(dpy, canvas,
				  vinfo.visual, menu.w, menu.h);
	cr = cairo_create(surf);

	PangoLayout *pangolayout = pango_cairo_create_layout(cr);
	PangoFontDescription *pangofont = pango_font_description_from_string("Sans");

	XMapWindow(dpy, win);

	cairo_save(cr);
	cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr);
	cairo_restore(cr);

	/* take screenshot */
	XImage *ximg = XGetImage(dpy, root, window_geo.x, window_geo.y,
				 window_geo.w, window_geo.h, AllPlanes, ZPixmap);
	stackblur(ximg, 0, 0, window_geo.w, window_geo.h, radius, 2);
	cairo_surface_t *s = surface_from_ximage(ximg, window_geo.w, window_geo.h);
	if (ximg)
		XDestroyImage(ximg);
	render_image(s, 0, 0, 0.99);

	double red[] = { 1.0, 0.0, 0.0, 1.0 };
	render_rectangle(&menu, red);
	XCopyArea(dpy, canvas, win, gc, 0, 0, menu.w, menu.h, 0, 0);

	/* main loop */
	XEvent e;
	for (;;)
	{
		if (XPending(dpy)) {
			XNextEvent(dpy, &e);
                        if (XFilterEvent(&e, win))
                                continue;
			switch (e.type) {
			case ButtonPress:
				goto out;
			case KeyPress:
				handle_key_event(&e.xkey);
				break;
			case MotionNotify:
				/* TODO: handle pointer motion */
				break;
			default:
				break;
			}
		}
	}

out:
	/* clean up */
	XDestroyWindow(dpy, win);
	XUngrabKeyboard(dpy, CurrentTime);
	XUngrabPointer(dpy, CurrentTime);
	if (xic)
		XDestroyIC(xic);
	XCloseIM(xim);
	if (canvas)
		XFreePixmap(dpy, canvas);
	if (gc)
		XFreeGC(dpy, gc);
	if (dpy)
		XCloseDisplay(dpy);
	cairo_destroy(cr);
	cairo_surface_destroy(surf);
	pango_font_description_free(pangofont);
	g_object_unref(pangolayout);
}
