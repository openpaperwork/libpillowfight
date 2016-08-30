/*
 * Copyright © 2016 Jerome Flesch
 *
 * Unpaper is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * Unpaper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

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
