#ifndef __PYPILLOWFIGHT_UTIL_H
#define __PYPILLOWFIGHT_UTIL_H

#include <stdint.h>

#include <Python.h>

/*!<
 * - Everything here assume we work with RGBA images
 * - In position (x, y): x = width, y = height.
 */

/*!
 * \returns a uint32_t (RGBA)
 */
#define GET_PIXEL(bitmap, a, b) ((bitmap)->pixels[((b) * (bitmap)->size.x) + (a)])

/*!
 * \returns a uint8_t
 */
#define GET_COLOR(bitmap, a, b, color) (GET_PIXEL(bitmap, a, b).channels[(color)])


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
