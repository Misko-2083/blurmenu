/*
 * blurmenu - an attempt to create a menu with a blurred background
 *
 * Copyright (C) 2021 Милош Павловић
 * Copyright (C) 2021 Johan Malm
 */

#include <getopt.h>
#include "blurmenu.h"
#include "stackblur.h"

static struct box blue_square_geo = { .x = 10, .y = 10, .w = 10, .h = 10 };

static const char blurmenu_usage[] =
"Usage: blurmenu [options]\n\n"
"  -h            Show help message and quit\n"
"  -r <radius>   Specify blur radius (1-50)\n"
"  -t <threads>  Specify CPU threads (2-16)\n"
"\n";

static void
usage(void)
{
	printf("%s", blurmenu_usage);
	exit(0);
}

void die(const char *fmt, ...)
{
	va_list params;
	va_start(params, fmt);
	vfprintf(stderr, fmt, params);
	va_end(params);
	exit(1);
}

void render_rectangle(cairo_t *cr, struct box *box, double *rgba, bool fill)
{
	double border_width = 1.0;
	double x = box->x + border_width / 2;
	double y = box->y + border_width / 2;
	double w = box->w - border_width;
	double h = box->h - border_width;

	cairo_set_source_rgba(cr, rgba[0], rgba[1], rgba[2], rgba[3]);
	cairo_set_line_width(cr, border_width);
	cairo_rectangle(cr, x, y, w, h);
	if (fill)
		cairo_fill(cr);
	cairo_stroke(cr);
}

static void render(struct xwindow *ctx, struct box *menu)
{
	cairo_save(ctx->cr);
	cairo_translate(ctx->cr, 0, 0);
	cairo_set_source_surface(ctx->cr, ctx->blurred_scrot, 0, 0);
	cairo_paint_with_alpha(ctx->cr, 0.99);
	cairo_restore(ctx->cr);

	double red[] = { 1.0, 0.0, 0.0, 1.0 };
	render_rectangle(ctx->cr, menu, red, false);

	double blue[] = { 0.0, 0.0, 1.0, 1.0 };
	render_rectangle(ctx->cr, &blue_square_geo, blue, true);

	XCopyArea(ctx->dpy, ctx->canvas, ctx->win, ctx->gc, 0, 0, menu->w,
		  menu->h, 0, 0);
}

static void handle_key_event(struct xwindow *ctx, XKeyEvent *ev)
{
	char buf[32];
	KeySym ksym = NoSymbol;
	Status status;

	Xutf8LookupString(ctx->xic, ev, buf, sizeof(buf), &ksym, &status);
	if (status == XBufferOverflow)
		return;

	int step = 5;
	switch (ksym) {
	case XK_Escape:
		exit(0);
	case XK_Up:
		blue_square_geo.y -= step;
		break;
	case XK_Down:
		blue_square_geo.y += step;
		break;
	case XK_Right:
		blue_square_geo.x += step;
		break;
	case XK_Left:
		blue_square_geo.x -= step;
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
	while ((c = getopt(argc, argv, "hr:t:")) != -1) {
		switch (c) {
		case 'r':
			radius = atoi(optarg);
			break;
		case 't':
			if (atoi(optarg) > 1 && atoi(optarg) < 17) {
				threads = atoi(optarg);
				break;
			}
		case 'h':
		default:
			usage();
		}
	}
	if (optind < argc) {
		usage();
	}

	struct xwindow ctx = { 0 };

	ctx.dpy = XOpenDisplay(NULL);
	if (!ctx.dpy)
		die("cannot open display");
	ctx.xim = XOpenIM(ctx.dpy, NULL, NULL, NULL);
	int screen = DefaultScreen(ctx.dpy);

	XVisualInfo vinfo;
	const char depths[] = { 32, 24, 8, 4, 2, 1, 0 };
	for (int i = 0; depths[i]; i++) {
		XMatchVisualInfo(ctx.dpy, screen, depths[i], TrueColor, &vinfo);
		if (vinfo.visual)
			break;
	}
	fprintf(stderr, "color-depth=%d\n", vinfo.depth);

	ctx.root = RootWindow(ctx.dpy, screen);

	x11_crt_geometry(&ctx, &ctx.screen_geo);

	/* local co-ordinates */
	struct box menu = { .x = 0, .y = 0, .w = 600, .h = 400 };

	ctx.window_geo.x = ctx.screen_geo.x + (ctx.screen_geo.w - menu.w) / 2;
	ctx.window_geo.y = ctx.screen_geo.y + (ctx.screen_geo.h - menu.h) / 2;
	ctx.window_geo.w = menu.w;
	ctx.window_geo.h = menu.h;

	x11_create_window(&ctx, &ctx.window_geo, &vinfo);

	if (XGrabKeyboard(ctx.dpy, ctx.root, True, GrabModeAsync, GrabModeAsync,
			  CurrentTime) != GrabSuccess)
		die("XGrabKeyboard");
	if (XGrabPointer(ctx.dpy, ctx.root, False,
			 ButtonPressMask | ButtonReleaseMask |
			 PointerMotionMask | FocusChangeMask |
			 EnterWindowMask | LeaveWindowMask,
			 GrabModeAsync, GrabModeAsync, None, None,
			 CurrentTime) != GrabSuccess)
		die("XGrabPointer");

	ctx.canvas = XCreatePixmap(ctx.dpy, ctx.root, menu.w, menu.h, vinfo.depth);
	cairo_surface_t *surf = cairo_xlib_surface_create(ctx.dpy, ctx.canvas,
				  vinfo.visual, menu.w, menu.h);
	ctx.cr = cairo_create(surf);

	PangoLayout *pangolayout = pango_cairo_create_layout(ctx.cr);
	PangoFontDescription *pangofont = pango_font_description_from_string("Sans");

	XMapWindow(ctx.dpy, ctx.win);

	cairo_save(ctx.cr);
	cairo_set_source_rgba(ctx.cr, 0.0, 0.0, 0.0, 0.0);
	cairo_set_operator(ctx.cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(ctx.cr);
	cairo_restore(ctx.cr);

	/* take screenshot */
	XImage *ximg = XGetImage(ctx.dpy, ctx.root, ctx.window_geo.x, ctx.window_geo.y,
				 ctx.window_geo.w, ctx.window_geo.h, AllPlanes, ZPixmap);
	stackblur(ximg, 0, 0, ctx.window_geo.w, ctx.window_geo.h, radius, threads);
	ctx.blurred_scrot = surface_from_ximage(ximg, ctx.window_geo.w, ctx.window_geo.h);
	if (ximg)
		XDestroyImage(ximg);

	render(&ctx, &menu);

	/* main loop */
	XEvent e;
	for (;;)
	{
		if (XPending(ctx.dpy)) {
			XNextEvent(ctx.dpy, &e);
                        if (XFilterEvent(&e, ctx.win))
                                continue;
			switch (e.type) {
			case ButtonPress:
				goto out;
			case KeyPress:
				handle_key_event(&ctx, &e.xkey);
				render(&ctx, &menu);
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
	XUngrabKeyboard(ctx.dpy, CurrentTime);
	XUngrabPointer(ctx.dpy, CurrentTime);
	cairo_surface_destroy(surf);
	cairo_surface_destroy(ctx.blurred_scrot);
	cairo_destroy(ctx.cr);
	pango_font_description_free(pangofont);
	g_object_unref(pangolayout);
	if (ctx.xic)
		XDestroyIC(ctx.xic);
	XCloseIM(ctx.xim);
	if (ctx.canvas)
		XFreePixmap(ctx.dpy, ctx.canvas);
	if (ctx.gc)
		XFreeGC(ctx.dpy, ctx.gc);
	XDestroyWindow(ctx.dpy, ctx.win);
}
