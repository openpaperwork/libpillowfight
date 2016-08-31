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
 * \brief Sobel filter
 *
 * Ref: https://www.researchgate.net/publication/239398674_An_Isotropic_3_3_Image_Gradient_Operator
 */

#define SIZE 3
#define LOW_THRESHOLD ((int)(WHITE * 0.47))
#define HIGH_THRESHOLD ((int)(WHITE * 0.61))

static void sobel_main(const struct bitmap *in, struct bitmap *out)
{
	memset(out->pixels, 0, sizeof(union pixel) * out->size.x * out->size.y);
	// TODO
}

static PyObject *sobel(PyObject *self, PyObject* args)
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
	sobel_main(&bitmap_in, &bitmap_out);

	PyBuffer_Release(&img_in);
	PyBuffer_Release(&img_out);
	Py_RETURN_NONE;
}

static PyMethodDef sobel_methods[] = {
	{"sobel", sobel, METH_VARARGS, NULL},
	{NULL, NULL, 0, NULL},
};

#if PY_VERSION_HEX < 0x03000000

PyMODINIT_FUNC
init_sobel(void)
{
    PyObject* m = Py_InitModule("_sobel", sobel_methods);
}

#else

static struct PyModuleDef sobel_module = {
	PyModuleDef_HEAD_INIT,
	"_sobel",
	NULL /* doc */,
	-1,
	sobel_methods,
};

PyMODINIT_FUNC PyInit__sobel(void)
{
	return PyModule_Create(&sobel_module);
}

#endif
