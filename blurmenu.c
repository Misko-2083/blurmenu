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


/*
   Select an area to save to "image.png" in the current folder
   gcc  blurmenu.c -Wall -o  blurmenu   `pkg-config --cflags --libs x11 xrender cairo` -lpthread
*/

/* stackblur.h */
typedef struct {
	unsigned char *pix;
	int x;
	int y;
	int w;
	int y2;
	int H;
	int wm;
	int wh;
	int *r;
	int *g;
	int *b;
	int *dv;
	int radius;
	int *vminx;
	int *vminy;
} StackBlurRenderingParams;

#include <pthread.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

void *HStackRenderingThread(void *arg);

void *VStackRenderingThread(void *arg);

void stackblur(XImage *image,int x, int y,int w,int h,int radius, unsigned int num_threads);
/* EOF stackblur.h */

/* stackblur.c */
void *HStackRenderingThread(void *arg) {
	StackBlurRenderingParams *rp=(StackBlurRenderingParams*)arg;
	int rinsum,ginsum,binsum,routsum,goutsum,boutsum,rsum,gsum,bsum,x,y,i,yi,yw,rbs,p,sp;
	int div=rp->radius+rp->radius+1;
	int *stackr=malloc(div*sizeof(int));
	int *stackg=malloc(div*sizeof(int));
	int *stackb=malloc(div*sizeof(int));
	yw=yi=rp->y*rp->w;
	int r1=rp->radius+1;
	for (y=rp->y;y<rp->y2;y++){
		rinsum=ginsum=binsum=routsum=goutsum=boutsum=rsum=gsum=bsum=0;
		for(i=-rp->radius;i<=rp->radius;i++){
			p=(yi+MIN(rp->wm,MAX(i,0)))*4;
			sp=i+rp->radius;
			stackr[sp]=rp->pix[p];
			stackg[sp]=rp->pix[p+1];
			stackb[sp]=rp->pix[p+2];
			rbs=r1-abs(i);
			rsum+=stackr[sp]*rbs;
			gsum+=stackg[sp]*rbs;
			bsum+=stackb[sp]*rbs;
			if (i>0){
				rinsum+=stackr[sp];
				ginsum+=stackg[sp];
				binsum+=stackb[sp];
			} else {
				routsum+=stackr[sp];
				goutsum+=stackg[sp];
				boutsum+=stackb[sp];
			}
		}
		int stackpointer;
		int stackstart;
		stackpointer=rp->radius;

		for (x=rp->x;x<rp->w;x++){
			rp->r[yi]=rp->dv[rsum];
			rp->g[yi]=rp->dv[gsum];
			rp->b[yi]=rp->dv[bsum];
			
			rsum-=routsum;
			gsum-=goutsum;
			bsum-=boutsum;

			stackstart=stackpointer-rp->radius+div;
			sp=stackstart%div;
			
			routsum-=stackr[sp];
			goutsum-=stackg[sp];
			boutsum-=stackb[sp];
			
			p=(yw+rp->vminx[x])*4;
			stackr[sp]=rp->pix[p];
			stackg[sp]=rp->pix[p+1];
			stackb[sp]=rp->pix[p+2];

			rinsum+=stackr[sp];
			ginsum+=stackg[sp];
			binsum+=stackb[sp];

			rsum+=rinsum;
			gsum+=ginsum;
			bsum+=binsum;
			
			stackpointer=(stackpointer+1)%div;
			sp=stackpointer%div;
			
			routsum+=stackr[sp];
			goutsum+=stackg[sp];
			boutsum+=stackb[sp];
			
			rinsum-=stackr[sp];
			ginsum-=stackg[sp];
			binsum-=stackb[sp];
			
			yi++;
		}
		yw+=rp->w;
	}
	free(stackr);
	free(stackg);
	free(stackb);
	stackr=stackg=stackb=NULL;
    pthread_exit(NULL);
}

void *VStackRenderingThread(void *arg) {
	StackBlurRenderingParams *rp=(StackBlurRenderingParams*)arg;
	int rinsum,ginsum,binsum,routsum,goutsum,boutsum,rsum,gsum,bsum,x,y,i,yi,yp,rbs,p,sp;
	int div=rp->radius+rp->radius+1;
	int divsum=(div+1)>>1;
	divsum*=divsum;
	int *stackr=malloc(div*sizeof(int));
	int *stackg=malloc(div*sizeof(int));
	int *stackb=malloc(div*sizeof(int));
	int r1=rp->radius+1;
	int hm=rp->H-rp->y-1;
	for (x=rp->x;x<rp->w;x++) {
		rinsum=ginsum=binsum=routsum=goutsum=boutsum=rsum=gsum=bsum=0;
		yp=(rp->y-rp->radius)*rp->w;
		for(i=-rp->radius;i<=rp->radius;i++) {
			yi=MAX(0,yp)+x;
			sp=i+rp->radius;

			stackr[sp]=rp->r[yi];
			stackg[sp]=rp->g[yi];
			stackb[sp]=rp->b[yi];
			
			rbs=r1-abs(i);
			
			rsum+=rp->r[yi]*rbs;
			gsum+=rp->g[yi]*rbs;
			bsum+=rp->b[yi]*rbs;
			
			if (i>0){
				rinsum+=stackr[sp];
				ginsum+=stackg[sp];
				binsum+=stackb[sp];
			} else {
				routsum+=stackr[sp];
				goutsum+=stackg[sp];
				boutsum+=stackb[sp];
			}
			
			if(i<hm){
				yp+=rp->w;
			}
		}
		yi=rp->y*rp->w+x;
		int stackpointer;
		int stackstart;
		stackpointer=rp->radius;

		for (y=rp->y;y<rp->y2;y++) {
			p=yi*4;
#ifdef DEBUG
// 		fprintf(stdout,"y: %i %i %i 1\n",rp->y, x, y);
#endif
 			rp->pix[p]=(unsigned char)(rp->dv[rsum]);
 			rp->pix[p+1]=(unsigned char)(rp->dv[gsum]);
 			rp->pix[p+2]=(unsigned char)(rp->dv[bsum]);
 			rp->pix[p+3]=0xff;
#ifdef DEBUG
// 		fprintf(stdout,"y: %i 2\n",rp->y);
#endif

			rsum-=routsum;
			gsum-=goutsum;
			bsum-=boutsum;

			stackstart=stackpointer-rp->radius+div;
			sp=stackstart%div;
			
			routsum-=stackr[sp];
			goutsum-=stackg[sp];
			boutsum-=stackb[sp];
			
			p=x+rp->vminy[y];
			stackr[sp]=rp->r[p];
			stackg[sp]=rp->g[p];
			stackb[sp]=rp->b[p];
			
			rinsum+=stackr[sp];
			ginsum+=stackg[sp];
			binsum+=stackb[sp];

			rsum+=rinsum;
			gsum+=ginsum;
			bsum+=binsum;

			stackpointer=(stackpointer+1)%div;
			
			routsum+=stackr[stackpointer];
			goutsum+=stackg[stackpointer];
			boutsum+=stackb[stackpointer];
			
			rinsum-=stackr[stackpointer];
			ginsum-=stackg[stackpointer];
			binsum-=stackb[stackpointer];

			yi+=rp->w;
		}
	}
	free(stackr);
	free(stackg);
	free(stackb);
	stackr=stackg=stackb=NULL;
    pthread_exit(NULL);
}

void stackblur(XImage *image,int x, int y,int w,int h,int radius, unsigned int num_threads) {
	if (radius<1)
		return;
	char *pix=image->data;
	int wh=w*h;
	int *r=malloc(wh*sizeof(int));
	int *g=malloc(wh*sizeof(int));
	int *b=malloc(wh*sizeof(int));
	int i;

	int div=radius+radius+1;
	int divsum=(div+1)>>1;
	divsum*=divsum;
	int *dv=malloc(256*divsum*sizeof(int));
	for (i=0;i<256*divsum;i++) {
		dv[i]=(i/divsum);
	}
	int *vminx=malloc(w*sizeof(int));
	for (i=0;i<w;i++)
		vminx[i]=MIN(i+radius+1,w-1);
	int *vminy=malloc(h*sizeof(int));
	for (i=0;i<h;i++)
		vminy[i]=MIN(i+radius+1,h-1)*w;

	pthread_t *pthh=malloc(num_threads*sizeof(pthread_t));
	StackBlurRenderingParams *rp=malloc(num_threads*sizeof(StackBlurRenderingParams));
	int threadY=y;
	int threadH=(h/num_threads);
 	for (i=0;i<num_threads;i++) {
		rp[i].pix=(unsigned char*)pix;
		rp[i].x=x;
		rp[i].w=w;
		rp[i].y=threadY;
		//Below "if" is to avoid vertical threads running on the same line when h/num_threads is not a round number i.e. 1080 lines / 16 threads = 67.5 lines!
		if (i==num_threads-1)//last turn
			rp[i].y2=y+h;
		else
			rp[i].y2=threadY+threadH;
 		rp[i].H=h;
		rp[i].wm=rp[i].w-1;
		rp[i].wh=wh;
		rp[i].r=r;
		rp[i].g=g;
		rp[i].b=b;
		rp[i].dv=dv;
		rp[i].radius=radius;
		rp[i].vminx=vminx;
		rp[i].vminy=vminy;
#ifdef DEBUG
		fprintf(stdout,"HThread: %i X: %i Y: %i W: %i H: %i x: %i y: %i w: %i h: %i\n",i,x,y,w,h,rp[i].x,rp[i].y,rp[i].w,threadH);
#endif
		pthread_create(&pthh[i],NULL,HStackRenderingThread,(void*)&rp[i]);
		threadY+=threadH;
	}
	for (i=0;i<num_threads;i++)
		pthread_join(pthh[i],NULL);
	pthread_t *pthv=malloc(num_threads*sizeof(pthread_t));
	for (i=0;i<num_threads;i++) {
#ifdef DEBUG
 		fprintf(stdout,"VThread: %i X: %i Y: %i W: %i H: %i x: %i y: %i w: %i h: %i\n",i,x,y,w,h,rp[i].x,rp[i].y,rp[i].w,threadH);
#endif
		pthread_create(&pthv[i],NULL,VStackRenderingThread,(void*)&rp[i]);
	}
	for (i=0;i<num_threads;i++)
		pthread_join(pthv[i],NULL);
	free(vminx);
	free(vminy);
	free(rp);
	free(r);
	free(g);
	free(b);
	free(dv);
	free(pthh);
	free(pthv);
	rp=NULL;
	dv=vminx=vminy=r=g=b=NULL;
	pthh=pthv=NULL;
#ifdef DEBUG
 	fprintf(stdout,"Done.\n");
#endif
}
/* EOF stackblur.c */

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

