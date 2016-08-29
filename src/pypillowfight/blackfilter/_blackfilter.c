#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <values.h>

#include <Python.h>

#include <util.h>

static void blackfilter_main(const struct bitmap *in, struct bitmap *out)
{
	memcpy(out, in, sizeof(struct bitmap));
}

static PyObject *blackfilter(PyObject *self, PyObject* args)
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

	blackfilter_main(&bitmap_in, &bitmap_out);

	PyBuffer_Release(&img_in);
	PyBuffer_Release(&img_out);
	Py_RETURN_NONE;
}

static PyMethodDef blackfilter_methods[] = {
	{"blackfilter", blackfilter, METH_VARARGS, NULL},
	{NULL, NULL, 0, NULL},
};

#if PY_VERSION_HEX < 0x03000000

PyMODINIT_FUNC
init_blackfilter(void)
{
    PyObject* m = Py_InitModule("_blackfilter", blackfilter_methods);
}

#else

static struct PyModuleDef blackfilter_module = {
	PyModuleDef_HEAD_INIT,
	"_blackfilter",
	NULL /* doc */,
	-1,
	blackfilter_methods,
};

PyMODINIT_FUNC PyInit__blackfilter(void)
{
	return PyModule_Create(&blackfilter_module);
}

#endif
