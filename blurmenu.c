/*
 * blurmenu - an attempt to create a menu with a blurred background
 *
 * Copyright (C) 2021 Милош Павловић
 * Copyright (C) 2021 Johan Malm
 */

#include <getopt.h>
#include "blurmenu.h"
#include "stackblur.h"

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

int main(int argc, char *argv[])
{
	struct box box = { .x = 10, .y = 10, .w = 100, .h = 100 };
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

	Display* dpy = XOpenDisplay(NULL);
	Screen *scr = ScreenOfDisplay(dpy, DefaultScreen(dpy));
	Window root = RootWindow(dpy, XScreenNumberOfScreen(scr));

	/* take screenshot */
	XImage *ximg = XGetImage(dpy, root, box.x, box.y, box.w, box.h,
				 AllPlanes, ZPixmap);
	stackblur(ximg, 0, 0, box.w, box.h, radius, 2);
	cairo_surface_t *surf = surface_from_ximage(ximg, box.w, box.h);
	if (ximg)
		XDestroyImage(ximg);

	cairo_surface_write_to_png(surf, "image.png");

	cairo_surface_destroy(surf);
	XCloseDisplay(dpy);
}
