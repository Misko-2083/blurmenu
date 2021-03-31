/*
 * blurmenu.c
 *
 * Copyright (C) 2021 Милош Павловић
 */

#include <cairo.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#include <X11/cursorfont.h>
#include <unistd.h>
#include "select.h"
#include "stackblur.h"

/*
   Select an area to save to "image.png" in the current folder
   gcc  blurmenu.c -Wall -o  blurmenu   `pkg-config --cflags --libs x11 xrender cairo` -lpthread
*/

// This is measured to be slightly faster.
#define GetPixel(ximg, x, y) ((u_int32_t *)&(ximg->data[y * ximg->bytes_per_line]))[x]

cairo_surface_t *get_root_ximage(int cordx, int cordy, int cordw, int cordh)
{
  /* function that gets the image from the root window and create cairo surface */
  //connect to x
  Display* dpy = XOpenDisplay(NULL);

  // Get root window 
  Screen *scr = NULL;
  scr = ScreenOfDisplay(dpy, DefaultScreen(dpy));

  Window root = 0;
  root = RootWindow(dpy, XScreenNumberOfScreen(scr));

  cairo_surface_t *surface = NULL;

  XImage *ximg;

  ximg = XGetImage(dpy, root, cordx, cordy, cordw, cordh, AllPlanes, ZPixmap);

  /* call the blur here  */
  stackblur(ximg,0,0,cordw,cordh,10, 2);

  XCloseDisplay(dpy);

  int stride;
  stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, cordw);

  unsigned char *data = malloc(stride * cordh);
  int x, y;

  for (y = 0; y < cordh; ++y)
      for (x = 0; x < cordw; ++x) {

          unsigned long pixel = GetPixel(ximg, x, y);

          unsigned char blue  = (ximg->blue_mask & pixel);
          unsigned char green = (ximg->green_mask & pixel) >> 8;
          unsigned char red   = (ximg->red_mask & pixel)>> 16;

          data[y * stride + x * 4 + 0] = blue;
          data[y * stride + x * 4 + 1] = green;
          data[y * stride + x * 4 + 2] = red;
      }

  surface = cairo_image_surface_create_for_data(
            data,
            CAIRO_FORMAT_RGB24,
            cordw, cordh,
            stride);


  cairo_surface_mark_dirty(surface);

  if (ximg) {
      XDestroyImage(ximg);
      ximg = NULL;
  }


return surface;
}

int main (int argc, char *argv[])
{
  
  int cordx, cordy, cordw, cordh;
  select_area (&cordx, &cordy, &cordw, &cordh);

  printf("X=%d\nY=%d\nWIDTH=%d\nHEIGHT=%d\n", cordx, cordy, cordw, cordh);

  cairo_surface_t *image_surface = NULL;
  image_surface = get_root_ximage(cordx, cordy, cordw, cordh);

  cairo_surface_write_to_png(image_surface, "image.png");

  cairo_surface_destroy(image_surface);

  return 0;
}

