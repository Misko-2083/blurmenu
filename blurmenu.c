/*
 * blurmenu - an attempt to create a menu with a blurred background
 *
 * Copyright (C) 2021 Милош Павловић
 * Copyright (C) 2021 Johan Malm
 */

#include <cairo.h>
#include <cairo-xlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <unistd.h>
#include "stackblur.h"

// This is measured to be slightly faster.
#define GetPixel(_ximg_, _x_, _y_) ((u_int32_t *)&(ximg->data[_y_ * _ximg_->bytes_per_line]))[_x_]

cairo_surface_t *covert_to_cairo(XImage *ximg, int width, int height)
{
	cairo_surface_t *surface = NULL;
	int stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, width);

	unsigned char *data = calloc(1, stride * height);

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			unsigned long pixel = XGetPixel(ximg, x, y);
			unsigned char blue = (ximg->blue_mask & pixel);
			unsigned char green = (ximg->green_mask & pixel) >> 8;
			unsigned char red = (ximg->red_mask & pixel) >> 16;
			data[y * stride + x * 4 + 0] = blue;
			data[y * stride + x * 4 + 1] = green;
			data[y * stride + x * 4 + 2] = red;
		}
	}

	surface = cairo_image_surface_create_for_data(data, CAIRO_FORMAT_RGB24,
						      width, height, stride);
	cairo_surface_mark_dirty(surface);
	return surface;
}

XImage *get_root_ximage(int x, int y, int w, int h)
{
	Display* dpy = XOpenDisplay(NULL);
	Screen *scr = ScreenOfDisplay(dpy, DefaultScreen(dpy));
	Window root = RootWindow(dpy, XScreenNumberOfScreen(scr));
	XImage *ximg = XGetImage(dpy, root, x, y, w, h, AllPlanes, ZPixmap);
	XCloseDisplay(dpy);
	return ximg;
}

int main(int argc, char *argv[])
{
	int x=10, y=10, w=100, h=100;
	int radius = 10;
	XImage *ximg = get_root_ximage(x, y, w, h);
	stackblur(ximg, 0, 0, w, h, radius, 2);
	cairo_surface_t *surf = covert_to_cairo(ximg, w, h);
	if (ximg)
		XDestroyImage(ximg);
	cairo_surface_write_to_png(surf, "image.png");
	cairo_surface_destroy(surf);
}

