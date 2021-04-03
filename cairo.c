/*
 * cairo.c -  cairo helpers
 *
 * Copyright (C) 2021 Милош Павловић
 * Copyright (C) 2021 Johan Malm
 */

#include "blurmenu.h"
#include "config.h"

cairo_surface_t *texture_create(struct box *box, const char *text, double *bg, double *fg)
{
	cairo_surface_t *surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
		box->w, box->h);
	cairo_t *cairo = cairo_create(surf);

	cairo_set_source_rgb(cairo, bg[0], bg[1], bg[2]);
	cairo_paint(cairo);
	cairo_set_source_rgba(cairo, fg[0], fg[1], fg[2], fg[3]);
	cairo_move_to(cairo, 0, 0);

	PangoLayout *layout = pango_cairo_create_layout(cairo);
	pango_layout_set_width(layout, box->w * PANGO_SCALE);
	pango_layout_set_text(layout, text, -1);

	PangoFontDescription *desc = pango_font_description_from_string(config.font);
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);
	pango_cairo_update_layout(cairo, layout);

	/* center-align vertically */
	int height;
	pango_layout_get_pixel_size(layout, NULL, &height);
	cairo_move_to(cairo, config.item_padding, (box->h - height) / 2);

	pango_cairo_show_layout(cairo, layout);
	g_object_unref(layout);

	cairo_surface_flush(surf);
	cairo_destroy(cairo);
	return surf;
}

cairo_surface_t *surface_from_ximage(XImage *ximg, int width, int height)
{
	int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
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
			data[y * stride + x * 4 + 3] = 255;
		}
	}

	cairo_surface_t *surface = NULL;
	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	unsigned char *surface_data = cairo_image_surface_get_data(surface);
	cairo_surface_flush(surface);
	memcpy(surface_data, data, width * height * 4);
	free(data);
	cairo_surface_mark_dirty(surface);
	return surface;
}
