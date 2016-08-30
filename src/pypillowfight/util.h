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

#ifndef __PYPILLOWFIGHT_UTIL_H
#define __PYPILLOWFIGHT_UTIL_H

#include <stdint.h>

#include <Python.h>

/*!<
 * - Everything here assume we work with RGBA images
 * - In position (x, y): x = width, y = height.
 */

#define COLOR_R 0
#define COLOR_G 1
#define COLOR_B 2
#define NB_COLORS 4 /* to align on 32bits */

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
 * \brief convert a py_buffer into a struct bitmap
 * \warning assumes the py_buffer is a RGBA image
 */
struct bitmap from_py_buffer(const Py_buffer *buffer, int x, int y);

Py_buffer to_py_buffer(const struct bitmap *bitmap);

#endif
