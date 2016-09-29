#include <pillowfight/pillowfight.h>

#include "_pymod.h"

static PyMethodDef clib_methods[] = {
	{"ace", pyace, METH_VARARGS, NULL},
	{"canny", pycanny, METH_VARARGS, NULL},
	{"compare", pycompare, METH_VARARGS, NULL},
	{"gaussian", pygaussian, METH_VARARGS, NULL},
	{"sobel", pysobel, METH_VARARGS, NULL},
	{"swt", pyswt, METH_VARARGS, NULL},
	{"unpaper_blackfilter", pyblackfilter, METH_VARARGS, NULL},
	{"unpaper_blurfilter", pyblurfilter, METH_VARARGS, NULL},
	{"unpaper_border", pyborder, METH_VARARGS, NULL},
	{"unpaper_grayfilter", pygrayfilter, METH_VARARGS, NULL},
	{"unpaper_masks", pymasks, METH_VARARGS, NULL},
	{"unpaper_noisefilter", pynoisefilter, METH_VARARGS, NULL},
	{NULL, NULL, 0, NULL},
};

#if PY_VERSION_HEX < 0x03000000

PyMODINIT_FUNC
init_clib(void)
{
    Py_InitModule("_clib", clib_methods);
}

#else

static struct PyModuleDef clib_module = {
	PyModuleDef_HEAD_INIT,
	"_clib",
	NULL /* doc */,
	-1,
	clib_methods,
};

PyMODINIT_FUNC PyInit__clib(void)
{
	return PyModule_Create(&clib_module);
}
#endif
