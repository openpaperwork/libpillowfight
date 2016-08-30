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
#include <stdlib.h>

#include "util.h"

const union pixel g_default_white_pixel = {
	.whole = WHOLE_WHITE,
};

struct bitmap from_py_buffer(const Py_buffer *buffer, int x, int y)
{
	struct bitmap out;

	assert(x * y * 4 == buffer->len);

	out.size.x = x;
	out.size.y = y;
	out.pixels = buffer->buf;

	return out;
}


void clear_rect(struct bitmap *img, int left, int top, int right, int bottom)
{
	int x;
	int y;

	if (left < 0)
		left = 0;
	if (top < 0)
		top = 0;
	if (right > img->size.x)
		right = img->size.x;
	if (bottom > img->size.y)
		bottom = img->size.y;

	for (y = top; y < bottom; y++) {
		for (x = left; x < right; x++) {
			SET_PIXEL(img, x, y, WHOLE_WHITE);
		}
	}
}


int count_pixels_rect(int left, int top, int right, int bottom,
		int max_brightness, const struct bitmap *img)
{
	int x;
	int y;
	int count = 0;
	int pixel;

	for (y = top; y <= bottom; y++) {
		for (x = left; x <= right; x++) {
			pixel = GET_PIXEL_GRAYSCALE(img, x, y);
			if ((pixel >= 0) && (pixel <= max_brightness)) {
				count++;
			}
		}
	}
	return count;
}


void apply_mask(struct bitmap *img, const struct rectangle *mask) {
	int x;
	int y;

	for (y=0 ; y < img->size.y ; y++) {
		for (x=0 ; x < img->size.x ; x++) {
			if (!(IS_IN(x, mask->a.x, mask->b.x) && IS_IN(y, mask->a.y, mask->b.y))) {
				SET_PIXEL(img, x, y, WHOLE_WHITE);
			}
		}
	}
}
