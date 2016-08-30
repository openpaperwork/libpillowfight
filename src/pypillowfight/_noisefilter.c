#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <values.h>

#include <Python.h>

#include "util.h"

/*!
 * \brief Algorithm noisefilter from unpaper, rewritten.
 */

static void noisefilter_main(const struct bitmap *in, struct bitmap *out)
{
	memcpy(out->pixels, in->pixels, sizeof(union pixel) * in->size.x * in->size.y);
	/* TODO */
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
