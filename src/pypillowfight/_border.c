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
#include <stdlib.h>
#include <string.h>
#include <values.h>

#include <pillowfight/pillowfight.h>
#include <pillowfight/util.h>

#ifndef NO_PYTHON
#include "_pymod.h"
#endif

/*!
 * \brief Algorithm 'border detection' from unpaper, partially rewritten.
 */

#define SCAN_SIZE 5
#define SCAN_STEP 5
#define SCAN_THRESHOLD 5
#define BLACK_THRESHOLD 0.33

#define ABS_BLACK_THRESHOLD ((int)(WHITE * (1.0 - BLACK_THRESHOLD)))


/* we only scan in the vertical direction */

/**
 * Find the size of one border edge.
 *
 * @param x1..y2 area inside of which border is to be detected
 */
static int detect_border_edge(const struct bitmap *img, int step_y) {
	int left;
	int right;
	int top;
	int bottom;
	int cnt;
	int result;

	// vertical detection
	left = 0;
	right = img->size.x;
	if (step_y > 0) {
		top = 0;
		bottom = SCAN_SIZE;
	} else {
		top = img->size.y - SCAN_SIZE;
		bottom = 0;
	}

	result = 0;
	while (result < img->size.y) {
		cnt = count_pixels_rect(left, top, right, bottom, ABS_BLACK_THRESHOLD, img);
		if (cnt >= SCAN_THRESHOLD) {
			return result; // border has been found: regular exit here
		}
		top += step_y;
		bottom += step_y;
		result += abs(step_y);
	}
	return 0; // no border found between 0..img->size.y
}


/**
 * Detects a border of completely non-black pixels
 */
static struct rectangle detect_border(const struct bitmap *img) {
	struct rectangle out;

	memset(&out, 0, sizeof(out));

	out.a.y += detect_border_edge(img, SCAN_STEP);
	out.b.y += detect_border_edge(img, -SCAN_STEP);

	out.b.x = img->size.x - out.b.x;
	out.b.y = img->size.y - out.b.y;

	return out;
}

#ifndef NO_PYTHON
static
#endif
void border(const struct bitmap *in, struct bitmap *out)
{
	struct rectangle border;

	memcpy(out->pixels, in->pixels, sizeof(union pixel) * in->size.x * in->size.y);

	border = detect_border(in);
	apply_mask(out, &border);
}

#ifndef NO_PYTHON
PyObject *pyborder(PyObject *self, PyObject* args)
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
	border(&bitmap_in, &bitmap_out);

	PyBuffer_Release(&img_in);
	PyBuffer_Release(&img_out);
	Py_RETURN_NONE;
}

#endif // !NO_PYTHON
