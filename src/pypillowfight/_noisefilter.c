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
#include <values.h>

#include <Python.h>

#include "util.h"

/*!
 * \brief Algorithm noisefilter from unpaper, partially rewritten.
 */

#define WHITE_THRESHOLD 0.9
#define WHITE_MIN ((int)(WHITE_THRESHOLD * WHITE))
#define INTENSITY 4

#define GET_PIXEL_LIGHTNESS(img, x, y) \
	MIN3( \
			GET_COLOR_DEF(img, x, y, COLOR_R, g_default_white_pixel), \
			GET_COLOR_DEF(img, x, y, COLOR_G, g_default_white_pixel), \
			GET_COLOR_DEF(img, x, y, COLOR_B, g_default_white_pixel) \
	)

static void callback_count(int x, int y, struct bitmap *img, void *cb_data) {
	int *count = cb_data;
	(*count)++;
}

static void callback_clear(int x, int y, struct bitmap *img, void *cb_data) {
	int *count = cb_data;
	(*count)++;
	SET_PIXEL(img, x, y, WHOLE_WHITE);
}

/**
 * Browse the number of dark pixels around the pixel at (x,y), who have a
 * square-metric distance of 'level' to the (x,y) (thus, testing the values
 * of 9 pixels if level==1, 16 pixels if level==2 and so on).
 * Optionally, the pixels can get cleared after counting.
 */
static void browse_pixel_neighbors_level(int x, int y,
		int level, struct bitmap *img,
		void (*callback)(int x, int y, struct bitmap *img, void *cb_data),
		void *cb_data) {
	int xx;
	int yy;
	int pixel;

	// upper and lower rows
	for (xx = x - level; xx <= x + level; xx++) {
		// upper row
		pixel = GET_PIXEL_LIGHTNESS(img, xx, y - level);
		if (pixel < WHITE_MIN) { // non-light pixel found
			callback(xx, y - level, img, cb_data);
		}
		// lower row
		pixel = GET_PIXEL_LIGHTNESS(img, xx, y + level);
		if (pixel < WHITE_MIN) {
			callback(xx, y + level, img, cb_data);
		}
	}
	// middle rows
	for (yy = y - (level - 1); yy <= y + (level - 1); yy++) {
		// first col
		pixel = GET_PIXEL_LIGHTNESS(img, x - level, yy);
		if (pixel < WHITE_MIN) { // non-light pixel found
			callback(x - level, yy, img, cb_data);
		}
		// last col
		pixel = GET_PIXEL_LIGHTNESS(img, x + level, yy);
		if (pixel < WHITE_MIN) {
			callback(x + level, yy, img, cb_data);
		}
	}
}


/**
 * Count all dark pixels in the distance 0..intensity that are directly
 * reachable from the dark pixel at (x,y), without having to cross bright
 * pixels.
 */
static int count_pixel_neighbors(int x, int y, struct bitmap *img) {
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
static void clear_pixel_neighbors(int x, int y, struct bitmap *img) {
	int level;
	int l_count = -1;

	SET_PIXEL(img, x, y, WHOLE_WHITE);

	// lCount will become 0, otherwise countPixelNeighbors() would
	// previously have delivered a bigger value (and this here would not
	// have been called)
	for (level = 1; l_count != 0; level++) {
		l_count = 0;
		browse_pixel_neighbors_level(x, y, level, img,
				callback_clear, &l_count);
	}
}

static void noisefilter_main(const struct bitmap *in, struct bitmap *out)
{
	int x;
	int y;
	int count;
	int pixel;
	int neighbors;

	memcpy(out->pixels, in->pixels, sizeof(union pixel) * in->size.x * in->size.y);

	count = 0;
	for (y = 0; y < out->size.y; y++) {
		for (x = 0; x < out->size.x; x++) {
			pixel = GET_PIXEL_DARKNESS_INVERSE(out, x, y);
			if (pixel < WHITE_MIN) { // one dark pixel found
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

static PyObject *noisefilter(PyObject *self, PyObject* args)
{
	int img_x, img_y;
	Py_buffer img_in, img_out;
	struct bitmap bitmap_in;
	struct bitmap bitmap_out;

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

	memset(bitmap_out.pixels, 0xFFFFFFFF, img_out.len);
	noisefilter_main(&bitmap_in, &bitmap_out);

	PyBuffer_Release(&img_in);
	PyBuffer_Release(&img_out);
	Py_RETURN_NONE;
}

static PyMethodDef noisefilter_methods[] = {
	{"noisefilter", noisefilter, METH_VARARGS, NULL},
	{NULL, NULL, 0, NULL},
};

#if PY_VERSION_HEX < 0x03000000

PyMODINIT_FUNC
init_noisefilter(void)
{
    PyObject* m = Py_InitModule("_noisefilter", noisefilter_methods);
}

#else

static struct PyModuleDef noisefilter_module = {
	PyModuleDef_HEAD_INIT,
	"_noisefilter",
	NULL /* doc */,
	-1,
	noisefilter_methods,
};

PyMODINIT_FUNC PyInit__noisefilter(void)
{
	return PyModule_Create(&noisefilter_module);
}

#endif
