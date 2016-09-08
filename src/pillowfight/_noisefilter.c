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
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <pillowfight/pillowfight.h>
#include <pillowfight/util.h>

#ifndef NO_PYTHON
#include "_pymod.h"
#endif

/*!
 * \brief Algorithm noisefilter from unpaper, partially rewritten.
 */

#define PF_WHITE_THRESHOLD 0.9
#define PF_WHITE_MIN ((int)(PF_WHITE_THRESHOLD * PF_WHITE))
#define INTENSITY 4

static void callback_count(int x, int y, struct pf_bitmap *img, void *cb_data) {
	int *count = cb_data;
	(*count)++;
}

static void callback_clear(int x, int y, struct pf_bitmap *img, void *cb_data) {
	int *count = cb_data;
	(*count)++;
	PF_SET_PIXEL(img, x, y, PF_WHOLE_WHITE);
}

/**
 * Browse the number of dark pixels around the pixel at (x,y), who have a
 * square-metric distance of 'level' to the (x,y) (thus, testing the values
 * of 9 pixels if level==1, 16 pixels if level==2 and so on).
 * Optionally, the pixels can get cleared after counting.
 */
static void browse_pixel_neighbors_level(int x, int y,
		int level, struct pf_bitmap *img,
		void (*callback)(int x, int y, struct pf_bitmap *img, void *cb_data),
		void *cb_data) {
	int xx;
	int yy;
	int pixel;

	// upper and lower rows
	for (xx = x - level; xx <= x + level; xx++) {
		// upper row
		pixel = PF_GET_PIXEL_LIGHTNESS(img, xx, y - level);
		if (pixel < PF_WHITE_MIN) { // non-light pixel found
			callback(xx, y - level, img, cb_data);
		}
		// lower row
		pixel = PF_GET_PIXEL_LIGHTNESS(img, xx, y + level);
		if (pixel < PF_WHITE_MIN) {
			callback(xx, y + level, img, cb_data);
		}
	}
	// middle rows
	for (yy = y - (level - 1); yy <= y + (level - 1); yy++) {
		// first col
		pixel = PF_GET_PIXEL_LIGHTNESS(img, x - level, yy);
		if (pixel < PF_WHITE_MIN) { // non-light pixel found
			callback(x - level, yy, img, cb_data);
		}
		// last col
		pixel = PF_GET_PIXEL_LIGHTNESS(img, x + level, yy);
		if (pixel < PF_WHITE_MIN) {
			callback(x + level, yy, img, cb_data);
		}
	}
}


/**
 * Count all dark pixels in the distance 0..intensity that are directly
 * reachable from the dark pixel at (x,y), without having to cross bright
 * pixels.
 */
static int count_pixel_neighbors(int x, int y, struct pf_bitmap *img) {
	int level;
	int count = 1; // assume self as set
	int l_count = -1;

	// can finish when one level is completely zero
	for (level = 1; (l_count != 0) && (level <= INTENSITY); level++) {
		l_count = 0;
		browse_pixel_neighbors_level(x, y, level, img,
				callback_count, &l_count);
		count += l_count;
	}
	return count;
}

/**
 * Clears all dark pixels that are directly reachable from the dark pixel at
 * (x,y). This should be called only if it has previously been detected that
 * the amount of pixels to clear will be reasonable small.
 */
static void clear_pixel_neighbors(int x, int y, struct pf_bitmap *img) {
	int level;
	int l_count = -1;

	PF_SET_PIXEL(img, x, y, PF_WHOLE_WHITE);

	// lCount will become 0, otherwise countPixelNeighbors() would
	// previously have delivered a bigger value (and this here would not
	// have been called)
	for (level = 1; l_count != 0; level++) {
		l_count = 0;
		browse_pixel_neighbors_level(x, y, level, img,
				callback_clear, &l_count);
	}
}

#ifndef NO_PYTHON
static
#endif
void pf_unpaper_noisefilter(const struct pf_bitmap *in, struct pf_bitmap *out)
{
	int x;
	int y;
	int count;
	int pixel;
	int neighbors;

	memcpy(out->pixels, in->pixels, sizeof(union pf_pixel) * in->size.x * in->size.y);

	count = 0;
	for (y = 0; y < out->size.y; y++) {
		for (x = 0; x < out->size.x; x++) {
			pixel = PF_GET_PIXEL_DARKNESS_INVERSE(out, x, y);
			if (pixel < PF_WHITE_MIN) { // one dark pixel found
				// get number of non-light pixels in neighborhood
				neighbors = count_pixel_neighbors(x, y, out);
				if (neighbors <= INTENSITY) { // not more than 'intensity'
					clear_pixel_neighbors(x, y, out); // delete area
					count++;
				}
			}
		}
	}
}

#ifndef NO_PYTHON

PyObject *pynoisefilter(PyObject *self, PyObject* args)
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
	pf_unpaper_noisefilter(&bitmap_in, &bitmap_out);
	Py_END_ALLOW_THREADS;

	PyBuffer_Release(&img_in);
	PyBuffer_Release(&img_out);
	Py_RETURN_NONE;
}

#endif // !NO_PYTHON
