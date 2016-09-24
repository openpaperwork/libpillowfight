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
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
#define LOW_THRESHOLD ((int)(PF_WHITE * 0.47))
#define HIGH_THRESHOLD ((int)(PF_WHITE * 0.61))

#define OUTPUT_INTERMEDIATE_IMGS 0

#if OUTPUT_INTERMEDIATE_IMGS == 1
#define DUMP_BITMAP(filename, bmp) pf_write_bitmap_to_ppm(filename ".ppm", bmp)
#define DUMP_MATRIX(filename, matrix, factor) pf_write_matrix_to_pgm(filename ".pgm", matrix, factor)
#else
#define DUMP_BITMAP(filename, bmp)
#define DUMP_MATRIX(filename, matrix, factor)
#endif

const double g_pf_kernel_sobel_x_values[] = {
    -1.0, 0.0, 1.0,
    -2.0, 0.0, 2.0,
    -1.0, 0.0, 1.0
};

const struct pf_dbl_matrix g_pf_kernel_sobel_x = {
	{ 3, 3 },
	(double *)g_pf_kernel_sobel_x_values,
};

const double g_pf_kernel_sobel_y_values[] = {
    -1.0, -2.0, -1.0,
    0.0, 0.0, 0.0,
    1.0, 2.0, 1.0,
};

const struct pf_dbl_matrix g_pf_kernel_sobel_y = {
	{ 3, 3, },
	(double *)g_pf_kernel_sobel_y_values,
};

const double g_pf_kernel_scharr_x_values[] = {
		3.0, 0.0, -3.0,
		10.0, 0.0, -10.0,
		3.0, 0.0, -3.0,
};

const struct pf_dbl_matrix g_pf_kernel_scharr_x = {
	{ 3, 3, },
	(double *)g_pf_kernel_scharr_x_values,
};

const double g_pf_kernel_scharr_y_values[] = {
		3.0, 10.0, 3.0,
		0.0, 0, 0.0,
		-3.0, -10.0, -3.0
};

const struct pf_dbl_matrix g_pf_kernel_scharr_y = {
	{ 3, 3, },
	(double *)g_pf_kernel_scharr_y_values,
};

/*!
 * \brief for each matrix point (x, y), compute the distance between (0, 0)
 *		and (matrix_a[x][y], matrix_b[x][y])
 * \param[in,out] a_out matrix used as input *and* output to save memory
 * \param[in] b matrix
 */
static struct pf_dbl_matrix compute_intensity_matrix(
		struct pf_dbl_matrix *matrix_a, const struct pf_dbl_matrix *matrix_b
	)
{
	int x, y;
	double a, b;
	double dist;
	struct pf_dbl_matrix out;

	assert(matrix_a->size.x == matrix_b->size.x);
	assert(matrix_a->size.y == matrix_b->size.y);

	out = pf_dbl_matrix_new(matrix_a->size.x, matrix_a->size.y);

	for (x = 0 ; x < matrix_a->size.x ; x++) {
		for (y = 0 ; y < matrix_a->size.y ; y++) {
			a = PF_MATRIX_GET(matrix_a, x, y);
			b = PF_MATRIX_GET(matrix_b, x, y);
			dist = hypot(a, b);
			PF_MATRIX_SET(&out, x, y, dist);
		}
	}

	return out;
}

static struct pf_dbl_matrix compute_direction_matrix(
		struct pf_dbl_matrix *matrix_a, const struct pf_dbl_matrix *matrix_b
	)
{
	int x, y;
	double val_a, val_b;
	double direction;
	struct pf_dbl_matrix out;

	assert(matrix_a->size.x == matrix_b->size.x);
	assert(matrix_a->size.y == matrix_b->size.y);

	out = pf_dbl_matrix_new(matrix_a->size.x, matrix_a->size.y);

	for (x = 0 ; x < matrix_a->size.x ; x++) {
		for (y = 0 ; y < matrix_a->size.y ; y++) {
			val_a = PF_MATRIX_GET(matrix_a, x, y);
			val_b = PF_MATRIX_GET(matrix_b, x, y);
			direction = atan2(val_b, val_a);
			PF_MATRIX_SET(&out, x, y, direction);
		}
	}

	return out;
}

struct pf_gradient_matrixes pf_sobel_on_matrix(const struct pf_dbl_matrix *in,
		const struct pf_dbl_matrix *kernel_x,
		const struct pf_dbl_matrix *kernel_y,
		double gaussian_sigma, int gaussian_stddev)
{
	struct pf_gradient_matrixes out;
	struct pf_dbl_matrix g_out;

	out.g_x = pf_dbl_matrix_convolution(in, kernel_x);
	DUMP_MATRIX("sobel_g_x", &out.g_x, 1.0);
	out.g_y = pf_dbl_matrix_convolution(in, kernel_y);
	DUMP_MATRIX("sobel_g_y", &out.g_y, 1.0);

	if (gaussian_stddev) {
		g_out = pf_gaussian_on_matrix(&out.g_x, gaussian_sigma, gaussian_stddev);
		pf_dbl_matrix_free(&out.g_x);
		memcpy(&out.g_x, &g_out, sizeof(out.g_x));
		g_out = pf_gaussian_on_matrix(&out.g_y, gaussian_sigma, gaussian_stddev);
		pf_dbl_matrix_free(&out.g_y);
		memcpy(&out.g_y, &g_out, sizeof(out.g_y));
	}

	out.intensity = compute_intensity_matrix(&out.g_x, &out.g_y);
	out.direction = compute_direction_matrix(&out.g_x, &out.g_y);

	return out;
}

#ifndef NO_PYTHON
static
#endif
void pf_sobel(const struct pf_bitmap *in_img, struct pf_bitmap *out_img)
{
	struct pf_dbl_matrix in;
	struct pf_dbl_matrix g_horizontal, g_vertical;
	struct pf_dbl_matrix intensity;

	in = pf_dbl_matrix_new(in_img->size.x, in_img->size.y);
	pf_rgb_bitmap_to_grayscale_dbl_matrix(in_img, &in);

	g_horizontal = pf_dbl_matrix_convolution(&in, PF_SOBEL_DEFAULT_KERNEL_X);
	g_vertical = pf_dbl_matrix_convolution(&in, PF_SOBEL_DEFAULT_KERNEL_Y);

	intensity = compute_intensity_matrix(&g_horizontal, &g_vertical);

	pf_dbl_matrix_free(&g_horizontal);
	pf_dbl_matrix_free(&g_vertical);
	pf_dbl_matrix_free(&in);

	pf_grayscale_dbl_matrix_to_rgb_bitmap(&intensity, out_img);
}

#ifndef NO_PYTHON

PyObject *pysobel(PyObject *self, PyObject* args)
{
	int img_x, img_y;
	Py_buffer img_in, img_out;
	struct pf_bitmap bitmap_in;
	struct pf_bitmap bitmap_out;

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
	Py_BEGIN_ALLOW_THREADS;
	pf_sobel(&bitmap_in, &bitmap_out);
	Py_END_ALLOW_THREADS;

	PyBuffer_Release(&img_in);
	PyBuffer_Release(&img_out);
	Py_RETURN_NONE;
}

#endif // !NO_PYTHON
