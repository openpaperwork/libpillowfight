#ifndef NO_PYTHON
#include "_pymod.h"
#endif
#include "_version.h"


#ifndef NO_PYTHON
static
#endif
const char *pf_get_version(void)
{
    return INTERNAL_PILLOWFIGHT_VERSION;
}


#ifndef NO_PYTHON
PyObject *pyget_version(PyObject *self, PyObject* args)
{
    return PyUnicode_FromString(pf_get_version());
}
#endif // !NO_PYTHON
