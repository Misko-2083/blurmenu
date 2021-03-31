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
#include "stackblur.h"

/*
   Select an area to save to "image.png" in the current folder
   gcc  blurmenu.c -Wall -o  blurmenu   `pkg-config --cflags --libs x11 xrender cairo` -lpthread
*/

static void _select_area (int * cordx, int * cordy, int * cordw, int * cordh)
{
  /* Funcion to select area
     https://bbs.archlinux.org/viewtopic.php?pid=660547#p660547
     We won't need this when there is a window mapped with x,y,width,height
  */
  int rx = 0, ry = 0, rw = 0, rh = 0;
  int rect_x = 0, rect_y = 0, rect_w = 0, rect_h = 0;
  int btn_pressed = 0, done = 0;

  XEvent ev;
  Display *disp = XOpenDisplay(NULL);

  if(!disp)
    exit(1);

  Screen *scr = NULL;
  scr = ScreenOfDisplay(disp, DefaultScreen(disp));

  Window root = 0;
  root = RootWindow(disp, XScreenNumberOfScreen(scr));

  Cursor cursor, cursor2;
  cursor = XCreateFontCursor(disp, XC_cross);
  cursor2 = XCreateFontCursor(disp, XC_crosshair);
  XColor color;
  Colormap colormap;
  char blueLight[] = "#0580FF"; // Misko decided to change the color
  colormap = DefaultColormap(disp, 0);
  XParseColor(disp, colormap, blueLight, &color);
  XAllocColor(disp, colormap, &color);

  XGCValues gcval;
  gcval.foreground = XWhitePixel(disp, 0);
  gcval.function = GXxor;
  gcval.background = XBlackPixel(disp, 0);
  gcval.plane_mask = gcval.background ^ gcval.foreground;
  gcval.subwindow_mode = IncludeInferiors;
  gcval.line_width = 2;

  GC gc;
  gc = XCreateGC(disp, root,
                 GCFunction | GCForeground | GCBackground | GCSubwindowMode | GCLineWidth ,
                 &gcval);

  /* this XGrab* stuff makes XPending true ? */
  if ((XGrabPointer
       (disp, root, False,
        ButtonMotionMask | ButtonPressMask | ButtonReleaseMask, GrabModeAsync,
        GrabModeAsync, root, cursor, CurrentTime) != GrabSuccess))
    printf("couldn't grab pointer:");

  if ((XGrabKeyboard
       (disp, root, False, GrabModeAsync, GrabModeAsync,
        CurrentTime) != GrabSuccess))
    printf("couldn't grab keyboard:");

  while (!done) {
//  while (!done && XPending(disp)) {
//    XNextEvent(disp, &ev);
// fixes the 100% CPU hog issue in original code
    if (!XPending(disp)) { usleep(1000); continue; }
    if ( (XNextEvent(disp, &ev) >= 0) ) {
      switch (ev.type) {
        case MotionNotify:
        /* this case is purely for drawing rect on screen */
          if (btn_pressed) {
            if (rect_w) {
              /* re-draw the last rect to clear it */
              XDrawRectangle(disp, root, gc, rect_x, rect_y, rect_w, rect_h);
            } else {
              /* Change the cursor to show we're selecting a region */
              XSetForeground(disp, gc, color.pixel);
              XChangeActivePointerGrab(disp,
                                       ButtonMotionMask | ButtonReleaseMask,
                                       cursor2, CurrentTime);
            }
            rect_x = rx;
            rect_y = ry;
            rect_w = ev.xmotion.x - rect_x;
            rect_h = ev.xmotion.y - rect_y;

            if (rect_w < 0) {
              rect_x += rect_w;
              rect_w = 0 - rect_w;
            }
            if (rect_h < 0) {
              rect_y += rect_h;
              rect_h = 0 - rect_h;
            }
            /* draw rectangle */
            XDrawRectangle(disp, root, gc, rect_x, rect_y, rect_w, rect_h);
            XFlush(disp);
          }
          break;
        case ButtonPress:
          btn_pressed = 1;
          rx = ev.xbutton.x;
          ry = ev.xbutton.y;
          break;
        case ButtonRelease:
          done = 1;
          break;
      }
    }
  }
  /* clear the drawn rectangle */
  if (rect_w) {
    XDrawRectangle(disp, root, gc, rect_x, rect_y, rect_w, rect_h);
    XFlush(disp);
  }
  rw = ev.xbutton.x - rx;
  rh = ev.xbutton.y - ry;
  /* cursor moves backwards */
  if (rw < 0) {
    rx += rw;
    rw = 0 - rw;
  }
  if (rh < 0) {
    ry += rh;
    rh = 0 - rh;
  }

  *cordx = rx;
  *cordy = ry;
  *cordw = rw;
  *cordh = rh;
  XCloseDisplay(disp);
}



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
  _select_area (&cordx, &cordy, &cordw, &cordh);

  printf("X=%d\nY=%d\nWIDTH=%d\nHEIGHT=%d\n", cordx, cordy, cordw, cordh);

  cairo_surface_t *image_surface = NULL;
  image_surface = get_root_ximage(cordx, cordy, cordw, cordh);

  cairo_surface_write_to_png(image_surface, "image.png");

  cairo_surface_destroy(image_surface);

  return 0;
}

