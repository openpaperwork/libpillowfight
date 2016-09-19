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

#ifndef __PILLOWFIGHT_UTIL_H
#define __PILLOWFIGHT_UTIL_H

#ifdef _WIN32
#define PF_WINDOWS
#endif

#ifdef PF_WINDOWS
#define _USE_MATH_DEFINES
#include <float.h>
#define MAXDOUBLE DBL_MAX
#else
#include <values.h>
#endif

#include <math.h>
#include <stdint.h>

#ifndef NO_PYTHON
#include <Python.h>
#endif

#include <pillowfight/pillowfight.h>

// Careful: double evaluation !
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

// Careful : multiple evaluation !
#ifndef MAX3
#define MAX3(a, b, c) MAX(a, MAX(b, c))
#endif
#ifndef MIN3
#define MIN3(a, b, c) MIN(a, MIN(b, c))
#endif

#define PF_IS_IN(v, a, b) ((a) <= (v) && (v) < (b))

// Careful : multiple evaluation !
#define PF_GET_PIXEL_DARKNESS_INVERSE(img, x, y) \
	MAX3( \
			PF_GET_PIXEL_DEF(img, x, y, g_pf_default_white_pixel).color.r, \
			PF_GET_PIXEL_DEF(img, x, y, g_pf_default_white_pixel).color.g, \
			PF_GET_PIXEL_DEF(img, x, y, g_pf_default_white_pixel).color.b \
		);

#define PF_GET_PIXEL_GRAYSCALE(img, x, y) \
	((PF_GET_PIXEL_DEF(img, x, y, g_pf_default_white_pixel).color.r \
	  + PF_GET_PIXEL_DEF(img, x, y, g_pf_default_white_pixel).color.g \
	  + PF_GET_PIXEL_DEF(img, x, y, g_pf_default_white_pixel).color.b) / 3)

#define PF_GET_PIXEL_LIGHTNESS(img, x, y) \
	MIN3( \
		PF_GET_COLOR_DEF(img, x, y, COLOR_R, g_pf_default_white_pixel), \
		PF_GET_COLOR_DEF(img, x, y, COLOR_G, g_pf_default_white_pixel), \
		PF_GET_COLOR_DEF(img, x, y, COLOR_B, g_pf_default_white_pixel) \
	)

#define PF_COUNT_OF(x) (sizeof(x) / sizeof(x[0]))


struct pf_rectangle {
	struct {
		int x;
		int y;
	} a;
	struct {
		int x;
		int y;
	} b;
};


extern const union pf_pixel g_pf_default_white_pixel;


#ifndef NO_PYTHON
/*!
 * \brief convert a py_buffer into a struct pf_bitmap
 * \warning assumes the py_buffer is a RGBA image
 */
struct pf_bitmap from_py_buffer(const Py_buffer *buffer, int x, int y);

Py_buffer to_py_buffer(const struct pf_bitmap *bitmap);
#endif


struct pf_dbl_matrix pf_dbl_matrix_new(int x, int y);
void pf_dbl_matrix_free(struct pf_dbl_matrix *matrix);

struct pf_dbl_matrix pf_dbl_matrix_copy(const struct pf_dbl_matrix *in);

/*!
 * \see https://en.wikipedia.org/wiki/Kernel_%28image_processing%29#Convolution
 */
struct pf_dbl_matrix pf_dbl_matrix_convolution(
		const struct pf_dbl_matrix *image,
		const struct pf_dbl_matrix *kernel
	);

struct pf_dbl_matrix dbl_matrix_transpose(const struct pf_dbl_matrix *in);

void pf_rgb_bitmap_to_grayscale_dbl_matrix(const struct pf_bitmap *in, struct pf_dbl_matrix *out);
void pf_grayscale_dbl_matrix_to_rgb_bitmap(const struct pf_dbl_matrix *in, struct pf_bitmap *out);

void pf_bitmap_channel_to_dbl_matrix(
		const struct pf_bitmap *in, struct pf_dbl_matrix *out, enum pf_color
);
void pf_matrix_to_rgb_bitmap(const struct pf_dbl_matrix *in, struct pf_bitmap *out, enum pf_color color);

/**
 * Clears a rectangular area of pixels with white.
 * @return The number of pixels actually changed from black (dark) to white.
 */
void pf_clear_rect(struct pf_bitmap *img, int left, int top, int right, int bottom);

/**
 * Counts the number of pixels in a rectangular area whose grayscale
 * values ranges between minColor and maxBrightness. Optionally, the area can get
 * cleared with white color while counting.
 */
int pf_count_pixels_rect(int left, int top, int right, int bottom,
		int max_brightness, const struct pf_bitmap *img);

/**
 * Permanently applies image mask. Each pixel which is not covered by at least
 * one mask is set to maskColor.
 */
void pf_apply_mask(struct pf_bitmap *img, const struct pf_rectangle *mask);

void pf_write_matrix_to_pgm(const char *filepath, const struct pf_dbl_matrix *in, double factor);

struct pf_dbl_matrix pf_normalize(const struct pf_dbl_matrix *in, double factor, double min, double max);
struct pf_dbl_matrix pf_grayscale_reverse(const struct pf_dbl_matrix *in);

#ifdef PF_WINDOWS
#define round(number) ((number) < 0.0 ? ceil((number) - 0.5) : floor((number) + 0.5))
#endif

#endif