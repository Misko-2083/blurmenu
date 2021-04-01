#include "blurmenu.h"

static bool is_point_on_crt(int x, int y, XRRCrtcInfo *ci)
{
	struct box b = {
		.x = (int)ci->x, .y = (int)ci->y,
		.w = (int)ci->width, .h = (int)ci->height,
	};
	if (x < b.x || x > b.x + b.w || y < b.y || y > b.y + b.h)
		return false;
	return true;
}

void x11_crt_geometry(struct xwindow *ctx, struct box *box)
{
	int i, n, x, y, di;
	unsigned int du;
	Window dw;
	XRRScreenResources *sr;
	XRRCrtcInfo *ci = NULL;
	sr = XRRGetScreenResourcesCurrent(ctx->dpy, ctx->root);
	n = sr->ncrtc;
	XQueryPointer(ctx->dpy, ctx->root, &dw, &dw, &x, &y, &di, &di, &du);
	for (i = 0; i < n; i++) {
		if (ci)
			XRRFreeCrtcInfo(ci);
		ci = XRRGetCrtcInfo(ctx->dpy, sr, sr->crtcs[i]);
		if (!ci->noutput)
			continue;
		if (is_point_on_crt(x, y, ci))
			break;
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

void x11_create_window(struct xwindow *ctx, struct box *box, XVisualInfo *vinfo)
{
	XSetWindowAttributes swa;
	swa.override_redirect = True;
	swa.event_mask = ExposureMask | KeyPressMask |
			 VisibilityChangeMask | ButtonPressMask;
	swa.colormap = XCreateColormap(ctx->dpy, ctx->root, vinfo->visual, AllocNone);
	swa.background_pixel = 0;
	swa.border_pixel = 0;
	ctx->win = XCreateWindow(ctx->dpy, ctx->root, box->x, box->y, box->w, box->h, 0,
		vinfo->depth, CopyFromParent, vinfo->visual, CWOverrideRedirect |
		CWColormap | CWBackPixel | CWEventMask | CWBorderPixel, &swa);
	ctx->xic = XCreateIC(ctx->xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
		XNClientWindow, ctx->win, XNFocusWindow, ctx->win, NULL);
	ctx->gc = XCreateGC(ctx->dpy, ctx->win, 0, NULL);
	XStoreName(ctx->dpy, ctx->win, "blurmenu");
	XSetIconName(ctx->dpy, ctx->win, "blurmenu");

	XClassHint *classhint = XAllocClassHint();
	classhint->res_name = (char *)"blurmenu";
	classhint->res_class = (char *)"blurmenu";
	XSetClassHint(ctx->dpy, ctx->win, classhint);
	XFree(classhint);

	XDefineCursor(ctx->dpy, ctx->win, XCreateFontCursor(ctx->dpy, 68));
	XSync(ctx->dpy, False);
}


