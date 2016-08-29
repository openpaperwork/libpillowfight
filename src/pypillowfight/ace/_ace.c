#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include <Python.h>

#include <util.h>

static void _ace(const struct bitmap *in, struct bitmap *out)
{

	memcpy(out->pixels, in->pixels, in->size.x * in->size.y * 4);

}

static PyObject *ace(PyObject *self, PyObject* args)
{
	int img_x, img_y;
	Py_buffer img_in, img_out;
	double slope, limit;
	int samples, seed;
	struct bitmap bitmap_in;
	struct bitmap bitmap_out;

	if (!PyArg_ParseTuple(args, "iiy*ddiiy*",
				&img_x, &img_y,
				&img_in, &slope, &limit,
				&samples, &seed, &img_out)) {
		return NULL;
	}

	assert(img_x * img_y * 4 /* RGBA */ == img_in.len);
	assert(img_in.len == img_out.len);

	bitmap_in = from_py_buffer(&img_in, img_x, img_y);
	bitmap_out = from_py_buffer(&img_out, img_x, img_y);

	_ace(&bitmap_in, &bitmap_out);

	PyBuffer_Release(&img_in);
	PyBuffer_Release(&img_out);
	Py_RETURN_NONE;
}


static PyMethodDef ace_methods[] = {
	{"ace", ace, METH_VARARGS, NULL},
	{NULL, NULL, 0, NULL},
};

static struct PyModuleDef ace_module = {
	PyModuleDef_HEAD_INIT,
	"_ace",
	NULL /* doc */,
	-1,
	ace_methods,
};

PyMODINIT_FUNC PyInit__ace(void)
{
	return PyModule_Create(&ace_module);
}
