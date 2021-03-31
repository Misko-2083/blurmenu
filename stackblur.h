#ifndef __STACKBLUR_H
#define __STACKBLUR_H

/*
 * stackblur.h
 *
 * Copied from https://github.com/aario/slock-blur
 * All credits go to
 *  - Aario Shahbany - https://github.com/aario
 *  - Mario Klingemann <mario@quasimondo.com>
 *    http://incubator.quasimondo.com/processing/fast_blur_deluxe.php
 *    The original source code of stack-blur:
 *    http://incubator.quasimondo.com/processing/stackblur.pde
 *    https://underdestruction.com/2004/02/25/stackblur-2004/
 *
 * MIT/X Consortium License
 *
 * Copyright (c) 2010 Mario Klingemann
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

//#define DEBUG

// Fast Gaussian Blur v1.3
// by Mario Klingemann <http://incubator.quasimondo.com>

// One of my first steps with Processing. I am a fan
// of blurring. Especially as you can use blurred images
// as a base for other effects. So this is something I
// might get back to in later experiments.
//
// What you see is an attempt to implement a Gaussian Blur algorithm
// which is exact but fast. I think that this one should be
// relatively fast because it uses a special trick by first
// making a horizontal blur on the original image and afterwards
// making a vertical blur on the pre-processed image-> This
// is a mathematical correct thing to do and reduces the
// calculation a lot.
//
// In order to avoid the overhead of function calls I unrolled
// the whole convolution routine in one method. This may not
// look nice, but brings a huge performance boost.
//
//
// v1.1: I replaced some multiplications by additions
//       and added aome minor pre-caclulations.
//       Also add correct rounding for float->int conversion
//
// v1.2: I completely got rid of all floating point calculations
//       and speeded up the whole process by using a
//       precalculated multiplication table. Unfortunately
//       a precalculated division table was becoming too
//       huge. But maybe there is some way to even speed
//       up the divisions.
//
// v1.3: Fixed a bug that caused blurs that start at y>0
//	 to go wrong. Thanks to Jeroen Schellekens for
//       finding it!

#include <pthread.h>
#include <X11/Xlib.h>

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

void *HStackRenderingThread(void *arg);

void *VStackRenderingThread(void *arg);

void stackblur(XImage *image,int x, int y,int w,int h,int radius, unsigned int num_threads);

#endif /* __STACKBLUR_H */
