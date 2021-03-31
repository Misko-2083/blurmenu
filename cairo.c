/*
 * cairo.c -  cairo helpers
 *
 * Copyright (C) 2021 Милош Павловић
 * Copyright (C) 2021 Johan Malm
 */

#include "blurmenu.h"

cairo_surface_t *surface_from_ximage(XImage *ximg, int width, int height)
{
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

	cairo_surface_t *surface = NULL;
	surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
	unsigned char *surface_data = cairo_image_surface_get_data(surface);
	cairo_surface_flush(surface);
	memcpy(surface_data, data, width * height * 4);
	free(data);
	cairo_surface_mark_dirty(surface);
	return surface;
}
