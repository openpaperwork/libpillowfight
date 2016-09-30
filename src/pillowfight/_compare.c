/*
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

#include <pillowfight/pillowfight.h>
#include <pillowfight/util.h>

#ifndef NO_PYTHON
#include "_pymod.h"
#endif

#define TOLERANCE 10


#ifndef NO_PYTHON
static
#endif
int pf_compare(const struct pf_bitmap *in, const struct pf_bitmap *in2,
		struct pf_bitmap *out, int tolerance)
{
	int x, y, ret;
	int value, value2;

	assert(in->size.x == in2->size.x);
	assert(in->size.y == in2->size.y);
	assert(in->size.x == out->size.x);
	assert(in->size.y == out->size.y);

	ret = 0;

	for (x = 0 ; x < out->size.x ; x++) {
		for (y = 0 ; y < out->size.y ; y++) {
			value = PF_GET_PIXEL_GRAYSCALE(in, x, y);
			value2 = PF_GET_PIXEL_GRAYSCALE(in2, x, y);

			if (abs(value - value2) <= TOLERANCE)
				value2 = value;

			PF_SET_COLOR(out, x, y, COLOR_A, 0xFF);
			if (value == value2) {
				PF_SET_COLOR(out, x, y, COLOR_R, value);
				PF_SET_COLOR(out, x, y, COLOR_G, value);
				PF_SET_COLOR(out, x, y, COLOR_B, value);
			} else {
				// make the red obvious
				PF_SET_COLOR(out, x, y, COLOR_R, 0xFF);
				PF_SET_COLOR(out, x, y, COLOR_G, (value + value2) / 4);
				PF_SET_COLOR(out, x, y, COLOR_B, (value + value2) / 4);
				ret++;
			}
		}
	}

	return ret;
}

#ifndef NO_PYTHON
PyObject *pycompare(PyObject *self, PyObject* args)
{
	int img_x, img_y;
	Py_buffer img_in, img_in2, img_out;
	int tolerance;
	struct pf_bitmap bitmap_in, bitmap_in2;
	struct pf_bitmap bitmap_out;
	int ret;

	if (!PyArg_ParseTuple(args, "iiy*y*y*i",
				&img_x, &img_y,
				&img_in, &img_in2,
				&img_out, &tolerance)) {
		return NULL;
	}

	assert(img_x * img_y * 4 /* RGBA */ == img_in.len);
	assert(img_x * img_y * 4 /* RGBA */ == img_in2.len);
	assert(img_in.len == img_out.len);

	bitmap_in = from_py_buffer(&img_in, img_x, img_y);
	bitmap_in2 = from_py_buffer(&img_in2, img_x, img_y);
	bitmap_out = from_py_buffer(&img_out, img_x, img_y);

	Py_BEGIN_ALLOW_THREADS;
	ret = pf_compare(&bitmap_in, &bitmap_in2, &bitmap_out, tolerance);
	Py_END_ALLOW_THREADS;

	PyBuffer_Release(&img_in);
	PyBuffer_Release(&img_in2);
	PyBuffer_Release(&img_out);

	return PyLong_FromLong(ret);
}

#endif // !NO_PYTHON
