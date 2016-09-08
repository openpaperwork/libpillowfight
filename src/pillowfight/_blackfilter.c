/*
 * Copyright © 2005-2007 Jens Gulden
 * Copyright © 2011-2011 Diego Elio Pettenò
 * Copyright © 2016 Jerome Flesch
 *
 * Pypillowfight is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * Pypillowfight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <pillowfight/pillowfight.h>
#include <pillowfight/util.h>


#ifndef NO_PYTHON
#include "_pymod.h"
#endif

/*!
 * \brief Algorithm blackfilter from unpaper, partially rewritten.
 */

#define THRESHOLD 0.33
#define SCAN_SIZE 20
#define SCAN_DEPTH 500
#define SCAN_STEP 5
#define SCAN_THRESHOLD 0.95
#define INTENSITY 20

#define ABS_SCAN_THRESHOLD (PF_WHITE * SCAN_THRESHOLD)
#define ABS_THRESHOLD (PF_WHITE * (1.0 - THRESHOLD))


static uint8_t get_darkness_rect(
			const int x1, const int y1,
			const int x2, const int y2,
			struct pf_bitmap *img
		) {
	unsigned int total = 0;
	const int count = (x2 - x1) * (y2 - y1);
	int x, y;

	for (y = y1; y < y2; y++) {
		for (x = x1; x < x2; x++) {
			total += PF_GET_PIXEL_DARKNESS_INVERSE(img, x, y);
		}
	}
	return PF_WHITE - (total / count);
}

/**
 * Solidly fills a line of pixels heading towards a specified direction
 * until color-changes in the pixels to overwrite exceed the 'intensity'
 * parameter.
 *
 * @param step_x either -1 or 1, if step_y is 0, else 0
 * @param step_y either -1 or 1, if step_x is 0, else 0
 */
static int fill_line(
		int x, int y,
		int step_x, int step_y,
		const struct pf_bitmap *img) {
	int distance = 0;
	int intensity_count = 1; // first pixel must match, otherwise directly exit
	uint8_t pixel;

	while (1) {
		x += step_x;
		y += step_y;
		pixel = PF_GET_PIXEL_GRAYSCALE(img, x, y);
		if ((pixel >= 0) && (pixel <= ABS_THRESHOLD)) {
			intensity_count = INTENSITY; // reset counter
		} else {
			// allow maximum of 'intensity' pixels to be bright, until stop
			intensity_count--;
		}
		if ((intensity_count > 0)
				&& (x >= 0)
				&& (x < img->size.x)
				&& (y >= 0)
				&& (y < img->size.y)) {
			PF_SET_PIXEL(img, x, y, PF_WHOLE_WHITE);
			distance++;
		} else {
			return distance; // exit here
		}
	}
}


static void flood_fill(int x, int y, struct pf_bitmap *img);


/**
 * Start flood-filling around the edges of a line which has previously been
 * filled using fillLine(). Here, the flood-fill algorithm performs its
 * indirect recursion.
 *
 * @param step_x either -1 or 1, if step_y is 0, else 0
 * @param step_y either -1 or 1, if step_x is 0, else 0
 * @see fill_line()
 */
static void flood_fill_around_line(
		int x, int y,
		int step_x, int step_y,
		int distance,
		struct pf_bitmap *img) {

	int d;

	for (d = 0 ; d < distance ; d++) {
		if (step_x != 0) {
			x += step_x;
			flood_fill(x, y + 1, img); // indirect recursion
			flood_fill(x, y - 1, img); // indirect recursion
		} else { // step_y != 0
			y += step_y;
			flood_fill(x + 1, y, img); // indirect recursion
			flood_fill(x - 1, y, img); // indirect recursion
		}
	}

}


static void flood_fill(int x, int y, struct pf_bitmap *img) {

	// is current pixel to be filled?
	const int pixel = PF_GET_PIXEL_GRAYSCALE(img, x, y);
	int left, top, right, bottom;

	if ((pixel >= 0) && (pixel <= ABS_THRESHOLD)) {
		// first, fill a 'cross' (both vertical, horizontal line)
		PF_SET_PIXEL(img, x, y, PF_WHOLE_WHITE);
		left = fill_line(x, y, -1, 0, img);
		top = fill_line(x, y, 0, -1, img);
		right = fill_line(x, y, 1, 0, img);
		bottom = fill_line(x, y, 0, 1, img);

		// now recurse on each neighborhood-pixel of the cross (most recursions will immediately return)
		flood_fill_around_line(x, y, -1, 0, left, img);
		flood_fill_around_line(x, y, 0, -1, top, img);
		flood_fill_around_line(x, y, 1, 0, right, img);
		flood_fill_around_line(x, y, 0, 1, bottom, img);
	}
}

static void blackfilter_scan(
		int step_x, int step_y,
		struct pf_bitmap *img)
{
	int left;
	int top;
	int right;
	int bottom;
	int shift_x;
	int shift_y;
	int l, t, r, b;
	int diff_x;
	int diff_y;
	uint8_t blackness;
	int x, y;
	uint8_t abs_scan_threshold = ABS_SCAN_THRESHOLD;

	left = 0;
	top = 0;

	if (step_x) { // horizontal scanning
		right = SCAN_SIZE - 1;
		bottom = SCAN_DEPTH - 1;
		shift_x = 0;
		shift_y = SCAN_DEPTH;
	} else { // vertical scanning
		right = SCAN_DEPTH - 1;
		bottom = SCAN_SIZE - 1;
		shift_x = SCAN_DEPTH;
		shift_y = 0;
	}

	for ( ;
			(left < img->size.x)
			&& (top < img->size.y) ;
			left += shift_x, top += shift_y, right += shift_x, bottom += shift_y
		) { // individual scanning "stripes" over the whole sheet

		l = left;
		t = top;
		r = right;
		b = bottom;

		// make sure last stripe does not reach outside sheet,
		// shift back inside (next +=shift will exit while-loop)
		if (r > img->size.x || b > img->size.y) {
			diff_x = r - img->size.x;
			diff_y = b - img->size.y;
			l -= diff_x;
			t -= diff_y;
			r -= diff_x;
			b -= diff_y;
		}

		for ( ;
				(l < img->size.x) && (t < img->size.y) ;
				l += step_x, t += step_y, r += step_x, b += step_y
			) {
			blackness = get_darkness_rect(l, t, r, b, img);
			if (blackness >= abs_scan_threshold) {
				// fill the black part with PF_WHITE
				for (y = t ; y < b ; y++) {
					for (x = l ; x < r; x++) {
						flood_fill(x, y, img);
					}
				}
			}

		}
	}
}

#ifndef NO_PYTHON
static
#endif
void pf_unpaper_blackfilter(const struct pf_bitmap *in, struct pf_bitmap *out)
{
	memcpy(out->pixels, in->pixels, sizeof(union pf_pixel) * in->size.x * in->size.y);

	blackfilter_scan(SCAN_STEP, 0, out);
	blackfilter_scan(0, SCAN_STEP, out);
}

#ifndef NO_PYTHON
PyObject *pyblackfilter(PyObject *self, PyObject* args)
{
	int img_x, img_y;
	Py_buffer img_in, img_out;
	struct pf_bitmap bitmap_in;
	struct pf_bitmap bitmap_out;

	if (!PyArg_ParseTuple(args, "iiy*y*",
				&img_x, &img_y,
				&img_in,
				&img_out)) {
		return NULL;
	}

	assert(img_x * img_y * 4 /* RGBA */ == img_in.len);
	assert(img_in.len == img_out.len);

	bitmap_in = from_py_buffer(&img_in, img_x, img_y);
	bitmap_out = from_py_buffer(&img_out, img_x, img_y);

	Py_BEGIN_ALLOW_THREADS;
	pf_unpaper_blackfilter(&bitmap_in, &bitmap_out);
	Py_END_ALLOW_THREADS;

	PyBuffer_Release(&img_in);
	PyBuffer_Release(&img_out);
	Py_RETURN_NONE;
}

#endif // !NO_PYTHON
