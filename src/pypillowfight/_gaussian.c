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
#include <values.h>

#ifdef NO_PYTHON
#include <pillowfight/pillowfight.h>
#else
#include "_pymod.h"
#endif

#include <pillowfight/util.h>

/*!
 * \brief Gaussian filter
 *
 */

#ifndef NO_PYTHON
static
#endif
void gaussian(const struct bitmap *in, struct bitmap *out, double sigma)
{
	memset(out->pixels, 0, sizeof(union pixel) * out->size.x * out->size.y);
	// TODO
}

#ifndef NO_PYTHON
PyObject *pygaussian(PyObject *self, PyObject* args)
{
	int img_x, img_y;
	Py_buffer img_in, img_out;
	struct bitmap bitmap_in;
	struct bitmap bitmap_out;
	double stddev;

	if (!PyArg_ParseTuple(args, "iiy*y*id",
				&img_x, &img_y,
				&img_in,
				&img_out,
				&stddev)) {
		return NULL;
	}

	assert(img_x * img_y * 4 /* RGBA */ == img_in.len);
	assert(img_in.len == img_out.len);

	bitmap_in = from_py_buffer(&img_in, img_x, img_y);
	bitmap_out = from_py_buffer(&img_out, img_x, img_y);

	memset(bitmap_out.pixels, 0xFFFFFFFF, img_out.len);
	gaussian(&bitmap_in, &bitmap_out, stddev);

	PyBuffer_Release(&img_in);
	PyBuffer_Release(&img_out);
	Py_RETURN_NONE;
}

#endif
