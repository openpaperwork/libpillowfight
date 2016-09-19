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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pillowfight/util.h>

#ifndef PF_WINDOWS
#include <values.h>
#endif


const union pf_pixel g_pf_default_white_pixel = {
	PF_WHOLE_WHITE,
};

#ifndef NO_PYTHON

struct pf_bitmap from_py_buffer(const Py_buffer *buffer, int x, int y)
{
	struct pf_bitmap out;

	assert(x * y * 4 == buffer->len);

	out.size.x = x;
	out.size.y = y;
	out.pixels = buffer->buf;

	return out;
}

#endif


void pf_clear_rect(struct pf_bitmap *img, int left, int top, int right, int bottom)
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
			PF_SET_PIXEL(img, x, y, PF_WHOLE_WHITE);
		}
	}
}


int pf_count_pixels_rect(int left, int top, int right, int bottom,
		int max_brightness, const struct pf_bitmap *img)
{
	int x;
	int y;
	int count = 0;
	int pixel;

	for (y = top; y <= bottom; y++) {
		for (x = left; x <= right; x++) {
			pixel = PF_GET_PIXEL_GRAYSCALE(img, x, y);
			if ((pixel >= 0) && (pixel <= max_brightness)) {
				count++;
			}
		}
	}
	return count;
}


void pf_apply_mask(struct pf_bitmap *img, const struct pf_rectangle *mask) {
	int x;
	int y;

	for (y=0 ; y < img->size.y ; y++) {
		for (x=0 ; x < img->size.x ; x++) {
			if (!(PF_IS_IN(x, mask->a.x, mask->b.x)
					&& PF_IS_IN(y, mask->a.y, mask->b.y))) {
				PF_SET_PIXEL(img, x, y, PF_WHOLE_WHITE);
			}
		}
	}
}


struct pf_dbl_matrix pf_dbl_matrix_new(int x, int y)
{
	struct pf_dbl_matrix out;
	out.size.x = x;
	out.size.y = y;
	out.values = calloc(x * y, sizeof(out.values[0]));
	return out;
}


void pf_dbl_matrix_free(struct pf_dbl_matrix *matrix)
{
	free(matrix->values);
}


struct pf_dbl_matrix pf_dbl_matrix_copy(const struct pf_dbl_matrix *in)
{
	struct pf_dbl_matrix out;

	out = pf_dbl_matrix_new(in->size.x, in->size.y);
	memcpy(out.values, in->values, in->size.x * in->size.y * sizeof(out.values[0]));

	return out;
}


struct pf_dbl_matrix dbl_matrix_transpose(const struct pf_dbl_matrix *in)
{
	struct pf_dbl_matrix out;
	int x, y;
	double val;

	out = pf_dbl_matrix_new(in->size.y, in->size.x);
	for (x = 0 ; x < in->size.x ; x++) {
		for (y = 0; y < in->size.y ; y++) {
			val = PF_MATRIX_GET(in, x, y);
			PF_MATRIX_SET(&out, y, x, val);
		}
	}

	return out;
}

/*!
 * Ref: https://en.wikipedia.org/wiki/Kernel_%28image_processing%29#Convolution
 * Ref: http://www.songho.ca/dsp/convolution/convolution2d_example.html
 */
struct pf_dbl_matrix pf_dbl_matrix_convolution(
		const struct pf_dbl_matrix *img,
		const struct pf_dbl_matrix *kernel)
{
	struct pf_dbl_matrix out;
	int img_y, kernel_y;
	int img_x, kernel_x;
	double img_val, kernel_val;
	double val;

	out = pf_dbl_matrix_new(img->size.x, img->size.y);

	for (img_x = 0 ; img_x < img->size.x ; img_x++) {
		for (img_y = 0 ; img_y < img->size.y ; img_y++) {

			val = 0;

			for (kernel_x = 0 ; kernel_x < kernel->size.x ; kernel_x++) {
				if (((img_x - kernel_x + (kernel->size.x / 2)) < 0)
						|| ((img_x - kernel_x + (kernel->size.x / 2)) >= img->size.x))
					break;

				for (kernel_y = 0 ; kernel_y < kernel->size.y ; kernel_y++) {

					if (((img_y - kernel_y + (kernel->size.y / 2)) < 0)
							|| ((img_y - kernel_y + (kernel->size.y / 2)) >= img->size.y))
						break;

					img_val = PF_MATRIX_GET(
							img,
							img_x - kernel_x + (kernel->size.x / 2),
							img_y - kernel_y + (kernel->size.y / 2)
						);

					kernel_val = PF_MATRIX_GET(
							kernel,
							kernel_x,
							kernel_y
						);

					val += (img_val * kernel_val);

				}
			}

			PF_MATRIX_SET(&out, img_x, img_y, val);
		}
	}

	return out;
}

void pf_rgb_bitmap_to_grayscale_dbl_matrix(const struct pf_bitmap *in, struct pf_dbl_matrix *out)
{
	int x, y;
	int value;

	assert(out->size.x == in->size.x);
	assert(out->size.y == in->size.y);

	for (x = 0 ; x < in->size.x ; x++) {
		for (y = 0 ; y < in->size.y ; y++) {
			value = PF_GET_PIXEL_GRAYSCALE(in, x, y);
			PF_MATRIX_SET(
				out, x, y,
				value
			);
		}
	}
}

void pf_grayscale_dbl_matrix_to_rgb_bitmap(const struct pf_dbl_matrix *in, struct pf_bitmap *out)
{
	int x, y;
	int value;

	assert(out->size.x == in->size.x);
	assert(out->size.y == in->size.y);

	for (x = 0 ; x < in->size.x ; x++) {
		for (y = 0 ; y < in->size.y ; y++) {
			value = PF_MATRIX_GET(in, x, y);
			if (value < 0)
				value = 0;
			if (value >= 256)
				value = 255;
			PF_SET_COLOR(out, x, y, COLOR_R, value);
			PF_SET_COLOR(out, x, y, COLOR_G, value);
			PF_SET_COLOR(out, x, y, COLOR_B, value);
			PF_SET_COLOR(out, x, y, COLOR_A, 0xFF);
		}
	}
}


void pf_bitmap_channel_to_dbl_matrix(
		const struct pf_bitmap *in, struct pf_dbl_matrix *out, enum pf_color color
)
{
	int x, y;
	int value;

	assert(out->size.x == in->size.x);
	assert(out->size.y == in->size.y);

	for (x = 0 ; x < in->size.x ; x++) {
		for (y = 0 ; y < in->size.y ; y++) {
			value = PF_GET_COLOR(in, x, y, color);
			PF_MATRIX_SET(
				out, x, y,
				value
			);
		}
	}
}


void pf_matrix_to_rgb_bitmap(const struct pf_dbl_matrix *in, struct pf_bitmap *out, enum pf_color color)
{
	int x, y;
	int value;

	assert(out->size.x == in->size.x);
	assert(out->size.y == in->size.y);

	for (x = 0 ; x < out->size.x ; x++) {
		for (y = 0 ; y < out->size.y ; y++) {
			value = PF_MATRIX_GET(in, x, y);
			if (value < 0)
				value = 0;
			if (value >= 256)
				value = 255;
			PF_SET_COLOR(out, x, y, color, value);
			PF_SET_COLOR(out, x, y, COLOR_A, 0xFF);
		}
	}
}

void pf_write_bitmap_to_ppm(const char *filepath, const struct pf_bitmap *in)
{
	FILE *fp;
	int x, y;
	int pixel;

	fp = fopen(filepath, "w");
	if (fp == NULL) {
		fprintf(stderr, "Failed to write [%s]: %d, %s\n",
				filepath, errno, strerror(errno));
	}

	fprintf(fp, "P6\n");
	fprintf(fp, "%d %d\n", in->size.x, in->size.y);
	fprintf(fp, "255\n");

	for (y = 0 ; y < in->size.y ; y++) {
		for (x = 0 ; x < in->size.x ; x++) {
			pixel = PF_GET_PIXEL(in, x, y).whole;
			fwrite(&pixel, 3, 1, fp);
		}
	}

	fclose(fp);
}

void pf_write_matrix_to_pgm(const char *filepath, const struct pf_dbl_matrix *in, double factor)
{
	FILE *fp;
	int x, y;
	double val;
	uint8_t val8;

	fp = fopen(filepath, "w");
	if (fp == NULL) {
		fprintf(stderr, "Failed to write [%s]: %d, %s\n",
				filepath, errno, strerror(errno));
	}

	fprintf(fp, "P5\n");
	fprintf(fp, "%d %d\n", in->size.x, in->size.y);
	fprintf(fp, "255\n");

	for (y = 0 ; y < in->size.y ; y++) {
		for (x = 0 ; x < in->size.x ; x++) {
			val = PF_MATRIX_GET(in, x, y);
			val *= factor;
			if (val > 255.0)
				val = 255.0;
			else if (val < 0)
				val = 0.0;
			val8 = val;
			fwrite(&val8, 1, 1, fp);
		}
	}

	fclose(fp);
}

struct pf_dbl_matrix pf_normalize(const struct pf_dbl_matrix *in, double factor, double out_min, double out_max)
{
	struct pf_dbl_matrix out;
	int x, y;
	double val;
	double in_min = out_min, in_max = out_max;

	if (factor == 0.0) {
		in_min = DBL_MAX;
		in_max = -DBL_MAX;
		for (x = 0; x < in->size.x ; x++) {
			for (y = 0 ; y < in->size.y ; y++) {
				val = PF_MATRIX_GET(in, x, y);
				in_min = MIN(in_min, val);
				in_max = MAX(in_max, val);
			}
		}

		factor = (out_max - out_min) / (in_max - in_min);
	}

	out = pf_dbl_matrix_new(in->size.x, in->size.y);
	for (x = 0; x < in->size.x ; x++) {
		for (y = 0 ; y < in->size.y ; y++) {
			val = PF_MATRIX_GET(in, x, y);
			val -= in_min;
			val *= factor;
			val += out_min;
			PF_MATRIX_SET(&out, x, y, val);
		}
	}

	return out;
}

struct pf_dbl_matrix pf_grayscale_reverse(const struct pf_dbl_matrix *in)
{
	struct pf_dbl_matrix out;
	int x, y;
	double val;
	double in_min, in_max;
	double factor;

	in_min = DBL_MAX;
	in_max = -DBL_MAX;
	for (x = 0; x < in->size.x ; x++) {
		for (y = 0 ; y < in->size.y ; y++) {
			val = PF_MATRIX_GET(in, x, y);
			in_min = MIN(in_min, val);
			in_max = MAX(in_max, val);
		}
	}

	factor = (in_min - in_max) / (in_max - in_min);

	out = pf_dbl_matrix_new(in->size.x, in->size.y);
	for (x = 0; x < in->size.x ; x++) {
		for (y = 0 ; y < in->size.y ; y++) {
			val = PF_MATRIX_GET(in, x, y);
			val *= factor;
			val += in_max;
			PF_MATRIX_SET(&out, x, y, val);
		}
	}

	return out;
}
