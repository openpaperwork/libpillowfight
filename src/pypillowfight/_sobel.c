/*
 * Copyright © 2005-2007 Jens Gulden
 * Copyright © 2011-2011 Diego Elio Pettenò
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
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <values.h>

#include <pillowfight/pillowfight.h>
#include <pillowfight/util.h>

#ifndef NO_PYTHON
#include "_pymod.h"
#endif

/*!
 * \brief Sobel filter
 *
 * Ref: https://www.researchgate.net/publication/239398674_An_Isotropic_3_3_Image_Gradient_Operator
 */

#define SIZE 3
#define LOW_THRESHOLD ((int)(WHITE * 0.47))
#define HIGH_THRESHOLD ((int)(WHITE * 0.61))

static const struct dbl_matrix g_kernel_x = {
	.size = {
		.x = 3,
		.y = 3,
	},
	.values = (double[]) {
		-1.0, 0, 1.0,
		-2.0, 0, 2.0,
		-1.0, 0, 1.0
	}
};

static const struct dbl_matrix g_kernel_y = {
	.size = {
		.x = 3,
		.y = 3,
	},
	.values = (double[]) {
		-1.0, -2.0, -1.0,
		0, 0, 0,
		1.0, 2.0, 1.0,
	}
};

/*!
 * \brief for each matrix point (x, y), compute the distance between (0, 0)
 *		and (matrix_a[x][y], matrix_b[x][y])
 * \param[in,out] a_out matrix used as input *and* output to save memory
 * \param[in] b matrix
 */
static void dist_matrix(struct dbl_matrix *matrix_a, const struct dbl_matrix *matrix_b)
{
	int x, y;
	int a, b;
	int dist;

	assert(matrix_a->size.x == matrix_b->size.x);
	assert(matrix_a->size.y == matrix_b->size.y);

	for (x = 0 ; x < matrix_a->size.x ; x++) {
		for (y = 0 ; y < matrix_a->size.y ; y++) {
			a = MATRIX_GET(matrix_a, x, y);
			b = MATRIX_GET(matrix_b, x, y);
			dist = sqrt((a * a) + (b * b));
			MATRIX_SET(matrix_a, x, y, dist);
		}
	}
}

#ifndef NO_PYTHON
static
#endif
void sobel(const struct bitmap *in_img, struct bitmap *out_img)
{
	struct dbl_matrix in;
	struct dbl_matrix g_horizontal, g_vertical;

	in = dbl_matrix_new(in_img->size.x, in_img->size.y);
	rgb_bitmap_to_grayscale_dbl_matrix(in_img, &in);

	g_horizontal = dbl_matrix_convolution(&in, &g_kernel_x);
	g_vertical = dbl_matrix_convolution(&in, &g_kernel_y);

	dist_matrix(&g_horizontal, &g_vertical);

	dbl_matrix_free(&g_vertical);
	dbl_matrix_free(&in);

	grayscale_dbl_matrix_to_rgb_bitmap(&g_horizontal, out_img);

	dbl_matrix_free(&g_horizontal);
}

#ifndef NO_PYTHON

PyObject *pysobel(PyObject *self, PyObject* args)
{
	int img_x, img_y;
	Py_buffer img_in, img_out;
	struct bitmap bitmap_in;
	struct bitmap bitmap_out;

	if (!PyArg_ParseTuple(args, "iiy*y*",
				&img_x, &img_y,
				&img_in,
				&img_out)) {
		return NULL;
	}

	assert(img_x * img_y * 4 /* RGBA */ == img_in.len);
	assert(img_in.len == img_out.len);

	bitmap_in = from_py_buffer(&img_in, img_x, img_y);
	bitmap_out = from_py_buffer(&img_out, img_x, img_y);

	memset(bitmap_out.pixels, 0xFFFFFFFF, img_out.len);
	sobel(&bitmap_in, &bitmap_out);

	PyBuffer_Release(&img_in);
	PyBuffer_Release(&img_out);
	Py_RETURN_NONE;
}

#endif // !NO_PYTHON
