#include <assert.h>
#include <stdlib.h>

#include "util.h"

struct bitmap from_py_buffer(const Py_buffer *buffer, int x, int y)
{
	struct bitmap out;

	assert(x * y * 4 == buffer->len);

	out.size.x = x;
	out.size.y = y;
	out.pixels = buffer->buf;

	return out;
}
