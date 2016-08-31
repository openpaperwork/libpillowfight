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

#include <pillowfight/util.h>

const union pixel g_default_white_pixel = {
	.whole = WHOLE_WHITE,
};

#ifndef NO_PYTHON

struct bitmap from_py_buffer(const Py_buffer *buffer, int x, int y)
{
	struct bitmap out;

	assert(x * y * 4 == buffer->len);

	out.size.x = x;
	out.size.y = y;
	out.pixels = buffer->buf;

	return out;
}

#endif


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


struct dbl_matrix dbl_matrix_new(int x, int y)
{
	struct dbl_matrix out;
	out.size.x = x;
	out.size.y = y;
	out.values = calloc(x * y, sizeof(out.values[0]));
	return out;
}


void dbl_matrix_free(struct dbl_matrix *matrix)
{
	free(matrix->values);
}

/*!
 * Ref: https://en.wikipedia.org/wiki/Kernel_%28image_processing%29#Convolution
 * Ref: http://www.songho.ca/dsp/convolution/convolution2d_example.html
 */
struct dbl_matrix dbl_matrix_convolution(
		const struct dbl_matrix *img,
		const struct dbl_matrix *kernel)
{
	struct dbl_matrix out;
	int img_y, kernel_y;
	int img_x, kernel_x;
	double img_val, kernel_val;
	double val;

	out = dbl_matrix_new(img->size.x, img->size.y);

	for (img_x = 0 ; img_x < img->size.x ; img_x++) {
		for (img_y = 0 ; img_y < img->size.y ; img_y++) {

			val = 0;

			for (kernel_x = 0 ; kernel_x < kernel->size.x ; kernel_x++) {
				if (img_x - kernel_x < 0)
					break;

				for (kernel_y = 0 ; kernel_y < kernel->size.y ; kernel_y++) {

					if (img_y - kernel_y < 0)
						break;

					img_val = MATRIX_GET(
							img,
							img_x - kernel_x,
							img_y - kernel_y
						);

					kernel_val = MATRIX_GET(
							kernel,
							kernel_x,
							kernel_y
						);

					val += (img_val * kernel_val);

				}
			}

			MATRIX_SET(&out, img_x, img_y, val);
		}
	}

	return out;
}

void rgb_bitmap_to_grayscale_dbl_matrix(const struct bitmap *in, struct dbl_matrix *out)
{
	int x, y;
	int value;

	assert(out->size.x == in->size.x);
	assert(out->size.y == in->size.y);

	for (x = 0 ; x < in->size.x ; x++) {
		for (y = 0 ; y < in->size.y ; y++) {
			value = GET_PIXEL_GRAYSCALE(in, x, y);
			MATRIX_SET(
				out, x, y,
				value
			);
		}
	}
}

void grayscale_dbl_matrix_to_rgb_bitmap(const struct dbl_matrix *in, struct bitmap *out)
{
	int x, y;
	int value;

	assert(out->size.x == in->size.x);
	assert(out->size.y == in->size.y);

	for (x = 0 ; x < in->size.x ; x++) {
		for (y = 0 ; y < in->size.y ; y++) {
			value = MATRIX_GET(in, x, y);
			if (value < 0)
				value = 0;
			if (value >= 256)
				value = 255;
			SET_COLOR(out, x, y, COLOR_R, value);
			SET_COLOR(out, x, y, COLOR_G, value);
			SET_COLOR(out, x, y, COLOR_B, value);
			SET_COLOR(out, x, y, COLOR_A, 0xFF);
		}
	}
}
