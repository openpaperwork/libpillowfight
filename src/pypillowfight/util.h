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

#ifndef __PYPILLOWFIGHT_UTIL_H
#define __PYPILLOWFIGHT_UTIL_H

#include <stdint.h>

#include <Python.h>

/*!<
 * - Everything here assume we work with RGBA images
 * - In position (x, y): x = width, y = height.
 */

enum color {
	COLOR_R = 0,
	COLOR_G,
	COLOR_B,
	COLOR_A,
};
#define NB_COLORS 4 /* to align on 32bits */

#define WHITE 0xFF
#define WHOLE_WHITE 0xFFFFFFFF

// Careful: double evaluation !
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

// Careful : multiple evaluation !
#define MAX3(a, b, c) MAX(a, MAX(b, c))
#define MIN3(a, b, c) MIN(a, MIN(b, c))

#define IS_IN(v, a, b) ((a) <= (v) && (v) < (b))

union pixel {
	uint32_t whole;
	struct {
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	} color;
	uint8_t channels[4];
};


struct bitmap {
	struct {
		size_t x;
		size_t y;
	} size;
	union pixel *pixels;
};

/*!
 * \returns a uint32_t (RGBA)
 */
#define GET_PIXEL(bitmap, a, b) ((bitmap)->pixels[((b) * (bitmap)->size.x) + (a)])
#define GET_PIXEL_DEF(bitmap, a, b, def) \
	((a < 0 || a >= bitmap->size.x) ? def : \
	 ((b < 0 || b >= bitmap->size.y) ? def : \
	  GET_PIXEL(bitmap, a, b)))

#define SET_PIXEL(bitmap, a, b, value) GET_PIXEL(bitmap, a, b).whole = (value);

/*!
 * \returns a uint8_t
 */
#define GET_COLOR(bitmap, a, b, color) (GET_PIXEL(bitmap, a, b).channels[(color)])
#define GET_COLOR_DEF(bitmap, a, b, color, def) (GET_PIXEL_DEF(bitmap, a, b, def).channels[(color)])

#define SET_COLOR(bitmap, a, b, color, value) (GET_PIXEL(bitmap, a, b).channels[(color)]) = (value)

// Careful : multiple evaluation !
#define GET_PIXEL_DARKNESS_INVERSE(img, x, y) \
	MAX3( \
			GET_PIXEL_DEF(img, x, y, g_default_white_pixel).color.r, \
			GET_PIXEL_DEF(img, x, y, g_default_white_pixel).color.g, \
			GET_PIXEL_DEF(img, x, y, g_default_white_pixel).color.b \
		);

#define GET_PIXEL_GRAYSCALE(img, x, y) \
	((GET_PIXEL_DEF(img, x, y, g_default_white_pixel).color.r \
	  + GET_PIXEL_DEF(img, x, y, g_default_white_pixel).color.g \
	  + GET_PIXEL_DEF(img, x, y, g_default_white_pixel).color.b) / 3)

#define GET_PIXEL_LIGHTNESS(img, x, y) \
	MIN3( \
		GET_COLOR_DEF(img, x, y, COLOR_R, g_default_white_pixel), \
		GET_COLOR_DEF(img, x, y, COLOR_G, g_default_white_pixel), \
		GET_COLOR_DEF(img, x, y, COLOR_B, g_default_white_pixel) \
	)

/*!
 * \brief matrix of integers
 */
struct int_matrix {
	struct {
		int x;
		int y;
	} size;
	int32_t *values;
};

#define INT_MATRIX_GET(matrix, a, b) ((matrix)->values[((b) * (matrix)->size.x) + (a)])
#define INT_MATRIX_SET(matrix, a, b, val) INT_MATRIX_GET(matrix, a, b) = (val);


struct rectangle {
	struct {
		int x;
		int y;
	} a;
	struct {
		int x;
		int y;
	} b;
};


extern const union pixel g_default_white_pixel;

/*!
 * \brief convert a py_buffer into a struct bitmap
 * \warning assumes the py_buffer is a RGBA image
 */
struct bitmap from_py_buffer(const Py_buffer *buffer, int x, int y);

Py_buffer to_py_buffer(const struct bitmap *bitmap);


struct int_matrix int_matrix_new(int x, int y);
void int_matrix_free(struct int_matrix *matrix);

/*!
 * \see https://en.wikipedia.org/wiki/Kernel_%28image_processing%29#Convolution
 */
struct int_matrix int_matrix_convolution(
		const struct int_matrix *image,
		const struct int_matrix *kernel
	);

void rgb_bitmap_to_grayscale_int_matrix(const struct bitmap *in, struct int_matrix *out);
void grayscale_int_matrix_to_rgb_bitmap(const struct int_matrix *in, struct bitmap *out);

/**
 * Clears a rectangular area of pixels with white.
 * @return The number of pixels actually changed from black (dark) to white.
 */
void clear_rect(struct bitmap *img, int left, int top, int right, int bottom);

/**
 * Counts the number of pixels in a rectangular area whose grayscale
 * values ranges between minColor and maxBrightness. Optionally, the area can get
 * cleared with white color while counting.
 */
int count_pixels_rect(int left, int top, int right, int bottom,
		int max_brightness, const struct bitmap *img);

/**
 * Permanently applies image mask. Each pixel which is not covered by at least
 * one mask is set to maskColor.
 */
void apply_mask(struct bitmap *img, const struct rectangle *mask);

#endif
