/*
 * blurmenu - an attempt to create a menu with a blurred background
 *
 * Copyright (C) 2021 Милош Павловић
 * Copyright (C) 2021 Johan Malm
 */

#include <errno.h>
#include <getopt.h>
#include <sys/select.h>
#include "blurmenu.h"
#include "menu.h"
#include "stackblur.h"

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

void render_image(struct xwindow *ctx, cairo_surface_t *image, double x, double y)
{
	cairo_save(ctx->cr);
	cairo_translate(ctx->cr, x, y);
	cairo_set_source_surface(ctx->cr, image, 0, 0);
	cairo_paint(ctx->cr);
	cairo_restore(ctx->cr);
}

static void render(struct xwindow *ctx, struct menu *menu)
{
	cairo_set_source_surface(ctx->cr, ctx->blurred_scrot, 0, 0);
	cairo_paint(ctx->cr);

	double red[] = { 1.0, 0.0, 0.0, 1.0 };
	render_rectangle(ctx->cr, &menu->box, red, false);

	struct menuitem *item;
	for (int i = 0; i < menu->nr_items; i++) {
		item = menu->items + i;
		render_image(ctx, item->texture, item->box.x, item->box.y);
		if (menu->selected == item)
			render_rectangle(ctx->cr, &item->box, red, false);
	}

	XCopyArea(ctx->dpy, ctx->canvas, ctx->win, ctx->gc, 0, 0, menu->box.w,
		  menu->box.h, 0, 0);
}

static void handle_key_event(struct xwindow *ctx, struct menu *menu, XKeyEvent *ev)
{
	char buf[32];
	KeySym ksym = NoSymbol;
	Status status;

	Xutf8LookupString(ctx->xic, ev, buf, sizeof(buf), &ksym, &status);
	if (status == XBufferOverflow)
		return;

	switch (ksym) {
	case XK_Escape:
		exit(0);
	case XK_Up:
		items_handle_key(menu, -1);
		break;
	case XK_Down:
		items_handle_key(menu, 1);
		break;
	case XK_Return:
	case XK_KP_Enter:
		printf("%s\n", menu->selected->name);
		exit(0);
	default:
		break;
	}
}

static void main_loop(struct xwindow *ctx, struct menu *menu)
{
	XEvent e;
	fd_set readfds;
	FD_ZERO(&readfds);
	int x11_fd = ConnectionNumber(ctx->dpy);
	int nfds = x11_fd + 1;
	FD_SET(x11_fd, &readfds);
	struct timeval tv = { 0 };

	for (;;) {
		FD_ZERO(&readfds);
		FD_SET(x11_fd, &readfds);

		/* timeout after 0.5 seconds */
		tv.tv_usec = 500000;
		int ret;
		if (!XPending(ctx->dpy))
			ret = select(nfds, &readfds, NULL, NULL, &tv);
		if (ret == -1 && errno == EINTR)
			continue;
		if (ret == -1)
			die("select()");

		if (XPending(ctx->dpy)) {
			XNextEvent(ctx->dpy, &e);
			if (XFilterEvent(&e, ctx->win))
				continue;
			switch (e.type) {
			case ButtonPress:
				return;
			case KeyPress:
				handle_key_event(ctx, menu, &e.xkey);
				render(ctx, menu);
				break;
			case MotionNotify:
				/* TODO: handle pointer motion */
				break;
			default:
				break;
			}
		}
	}
}

int main(int argc, char *argv[])
{
	int radius = 10;
	int threads = 2;
	int c;
	while ((c = getopt(argc, argv, "hr:t:")) != -1) {
		switch (c) {
		case 'r':
			radius = atoi(optarg);
			break;
		case 't':
			if (atoi(optarg) > 1 && atoi(optarg) < 17)
				threads = atoi(optarg);
			break;
		case 'h':
		default:
			usage();
		}
	}
	if (optind < argc)
		usage();

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
	ctx.root = RootWindow(ctx.dpy, screen);
	x11_crt_geometry(&ctx, &ctx.screen_geo);

	struct menu menu = { 0 };
	menu.box.w = 640;
	menu.box.h = 480;
	items_init(&menu);

	ctx.window_geo.x = ctx.screen_geo.x + (ctx.screen_geo.w - menu.box.w) / 2;
	ctx.window_geo.y = ctx.screen_geo.y + (ctx.screen_geo.h - menu.box.h) / 2;
	ctx.window_geo.w = menu.box.w;
	ctx.window_geo.h = menu.box.h;

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

	ctx.canvas = XCreatePixmap(ctx.dpy, ctx.root, menu.box.w, menu.box.h, vinfo.depth);
	cairo_surface_t *surf = cairo_xlib_surface_create(ctx.dpy, ctx.canvas,
				  vinfo.visual, menu.box.w, menu.box.h);
	ctx.cr = cairo_create(surf);

	/* take screenshot */
	XImage *ximg = XGetImage(ctx.dpy, ctx.root, ctx.window_geo.x, ctx.window_geo.y,
				 ctx.window_geo.w, ctx.window_geo.h, AllPlanes, ZPixmap);
	stackblur(ximg, 0, 0, ctx.window_geo.w, ctx.window_geo.h, radius, threads);
	ctx.blurred_scrot = surface_from_ximage(ximg, ctx.window_geo.w, ctx.window_geo.h);
	if (ximg)
		XDestroyImage(ximg);

	XMapWindow(ctx.dpy, ctx.win);
	render(&ctx, &menu);

	main_loop(&ctx, &menu);

	/* clean up */
	items_finish(&menu);
	XUngrabKeyboard(ctx.dpy, CurrentTime);
	XUngrabPointer(ctx.dpy, CurrentTime);
	cairo_surface_destroy(surf);
	cairo_surface_destroy(ctx.blurred_scrot);
	cairo_destroy(ctx.cr);
	pango_cairo_font_map_set_default(NULL);
	if (ctx.xic)
		XDestroyIC(ctx.xic);
	XCloseIM(ctx.xim);
	if (ctx.canvas)
		XFreePixmap(ctx.dpy, ctx.canvas);
	if (ctx.gc)
		XFreeGC(ctx.dpy, ctx.gc);
	XDestroyWindow(ctx.dpy, ctx.win);
}
