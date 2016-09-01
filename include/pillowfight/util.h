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

#include <stdint.h>

#ifndef NO_PYTHON
#include <Python.h>
#endif

/*!<
 * - Everything here assume we work with RGBA images
 * - In position (x, y): x = width, y = height.
 */

enum pf_color {
	COLOR_R = 0,
	COLOR_G,
	COLOR_B,
	COLOR_A,
};
#define PF_NB_COLORS 4 /* to align on 32bits */
#define PF_NB_RGB_COLORS 3 /* when we want explicitly to ignore the alpha channel */

#define PF_WHITE 0xFF
#define PF_WHOLE_WHITE 0xFFFFFFFF

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

#define IS_IN(v, a, b) ((a) <= (v) && (v) < (b))

union pf_pixel {
	uint32_t whole;
	struct {
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	} color;
	uint8_t channels[4];
};


struct pf_bitmap {
	struct {
		size_t x;
		size_t y;
	} size;
	union pf_pixel *pixels;
};

/*!
 * \returns a uint32_t (RGBA)
 */
#define PF_GET_PIXEL(bitmap, a, b) ((bitmap)->pixels[((b) * (bitmap)->size.x) + (a)])
#define PF_GET_PIXEL_DEF(bitmap, a, b, def) \
	((a < 0 || a >= bitmap->size.x) ? def : \
	 ((b < 0 || b >= bitmap->size.y) ? def : \
	  PF_GET_PIXEL(bitmap, a, b)))

#define PF_SET_PIXEL(bitmap, a, b, value) PF_GET_PIXEL(bitmap, a, b).whole = (value);

/*!
 * \returns a uint8_t
 */
#define PF_GET_COLOR(bitmap, a, b, color) (PF_GET_PIXEL(bitmap, a, b).channels[(color)])
#define PF_GET_COLOR_DEF(bitmap, a, b, color, def) (PF_GET_PIXEL_DEF(bitmap, a, b, def).channels[(color)])

#define PF_SET_COLOR(bitmap, a, b, color, value) (PF_GET_PIXEL(bitmap, a, b).channels[(color)]) = (value)

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

/*!
 * \brief matrix of integers
 */
struct pf_dbl_matrix {
	struct {
		int x;
		int y;
	} size;
	double *values;
};

#define PF_MATRIX_GET(matrix, a, b) ((matrix)->values[((b) * (matrix)->size.x) + (a)])
#define PF_MATRIX_SET(matrix, a, b, val) PF_MATRIX_GET(matrix, a, b) = (val);


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

/*!
 * \see https://en.wikipedia.org/wiki/Kernel_%28image_processing%29#Convolution
 */
struct pf_dbl_matrix pf_dbl_matrix_convolution(
		const struct pf_dbl_matrix *image,
		const struct pf_dbl_matrix *kernel
	);

struct pf_dbl_matrix dbl_matrix_transpose(const struct pf_dbl_matrix *in);

void pf_rgb_bitmap_grayscale_dbl_matrix(const struct pf_bitmap *in, struct pf_dbl_matrix *out);
void pf_grayscale_dbl_matrix_to_rgb_bitmap(const struct pf_dbl_matrix *in, struct pf_bitmap *out);

void pf_bitmap_channel_to_dbl_matrix(
		const struct pf_bitmap *in, struct pf_dbl_matrix *out, enum pf_color
);
void pf_matrix_to_rgb_bitmap(const struct pf_dbl_matrix *in, struct pf_bitmap *out, enum pf_color color);

/**
 * Clears a rectangular area of pixels with PF_WHITE.
 * @return The number of pixels actually changed from black (dark) to PF_WHITE.
 */
void pf_clear_rect(struct pf_bitmap *img, int left, int top, int right, int bottom);

/**
 * Counts the number of pixels in a rectangular area whose grayscale
 * values ranges between minColor and maxBrightness. Optionally, the area can get
 * cleared with PF_WHITE color while counting.
 */
int pf_count_pixels_rect(int left, int top, int right, int bottom,
		int max_brightness, const struct pf_bitmap *img);

/**
 * Permanently applies image mask. Each pixel which is not covered by at least
 * one mask is set to maskColor.
 */
void pf_apply_mask(struct pf_bitmap *img, const struct pf_rectangle *mask);

#endif
