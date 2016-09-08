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
 * \brief Gaussian filter
 *
 */

struct pf_dbl_matrix generate_gaussian_1d_kernel(double sigma, int nb_stddev)
{
	struct pf_dbl_matrix out;
	int pos;
	double x;
	double val;

	if (sigma == 0.0) {
		sigma = 0.3 * ((nb_stddev - 1) * 0.5 - 1) + 0.8;
	}

	out = pf_dbl_matrix_new(nb_stddev, 1);

	// Basic gaussian function
	for (pos = 0 ; pos < nb_stddev ; pos++) {
		x = ((int)(pos - (nb_stddev / 2)));
		val = 1 / sqrt(M_PI * 2 * sigma * sigma);
		val *= exp((-x * x) / (2 * sigma * sigma));
		PF_MATRIX_SET(&out, pos, 0, val);
	}
	for (pos = 0 ; pos < nb_stddev ; pos++) {
		val = PF_MATRIX_GET(&out, pos, 0);
	}

	// Normalize
	x = 0.0;
	for (pos = 0 ; pos < nb_stddev ; pos++) {
		val = PF_MATRIX_GET(&out, pos, 0);
		x += val;
	}
	x = 1.0 / x;
	for (pos = 0 ; pos < nb_stddev ; pos++) {
		val = PF_MATRIX_GET(&out, pos, 0);
		val *= x;
		PF_MATRIX_SET(&out, pos, 0, val);
	}

	return out;
}

struct pf_dbl_matrix pf_gaussian_on_matrix(
		const struct pf_dbl_matrix *grayscale_matrix, double sigma, int nb_stddev)
{
	struct pf_dbl_matrix kernel_x, kernel_y;
	struct pf_dbl_matrix out_x, out_y;

	kernel_x = generate_gaussian_1d_kernel(sigma, nb_stddev);
	kernel_y = dbl_matrix_transpose(&kernel_x);

	out_x = pf_dbl_matrix_convolution(grayscale_matrix, &kernel_x);
	pf_dbl_matrix_free(&kernel_x);

	out_y = pf_dbl_matrix_convolution(&out_x, &kernel_y);
	pf_dbl_matrix_free(&out_x);
	pf_dbl_matrix_free(&kernel_y);

	return out_y;
}

#ifndef NO_PYTHON
static
#endif
void pf_gaussian(const struct pf_bitmap *in, struct pf_bitmap *out, double sigma, int nb_stddev)
{
	struct pf_dbl_matrix kernel_x, kernel_y;
	struct pf_dbl_matrix colors[PF_NB_RGB_COLORS];
	struct pf_dbl_matrix color_out;
	enum pf_color color;

	kernel_x = generate_gaussian_1d_kernel(sigma, nb_stddev);
	kernel_y = dbl_matrix_transpose(&kernel_x);

	for (color = 0 ; color < PF_NB_RGB_COLORS ; color++) {
		colors[color] = pf_dbl_matrix_new(in->size.x, in->size.y);
		pf_bitmap_channel_to_dbl_matrix(in, &colors[color], color);

		// x
		color_out = pf_dbl_matrix_convolution(&colors[color], &kernel_x);
		pf_dbl_matrix_free(&colors[color]);
		colors[color] = color_out;

		// y
		color_out = pf_dbl_matrix_convolution(&colors[color], &kernel_y);
		pf_dbl_matrix_free(&colors[color]);
		colors[color] = color_out;
	}

	pf_dbl_matrix_free(&kernel_x);
	pf_dbl_matrix_free(&kernel_y);

	for (color = 0 ; color < PF_NB_RGB_COLORS ; color++) {
		pf_matrix_to_rgb_bitmap(&colors[color], out, color);
		pf_dbl_matrix_free(&colors[color]);
	}
}

#ifndef NO_PYTHON
PyObject *pygaussian(PyObject *self, PyObject* args)
{
	int img_x, img_y;
	Py_buffer img_in, img_out;
	struct pf_bitmap bitmap_in;
	struct pf_bitmap bitmap_out;
	double sigma;
	int nb_stddev;

	if (!PyArg_ParseTuple(args, "iiy*y*di",
				&img_x, &img_y,
				&img_in,
				&img_out,
				&sigma,
				&nb_stddev)) {
		return NULL;
	}

	assert(img_x * img_y * 4 /* RGBA */ == img_in.len);
	assert(img_in.len == img_out.len);

	bitmap_in = from_py_buffer(&img_in, img_x, img_y);
	bitmap_out = from_py_buffer(&img_out, img_x, img_y);

	memset(bitmap_out.pixels, 0xFFFFFFFF, img_out.len);
	Py_BEGIN_ALLOW_THREADS;
	pf_gaussian(&bitmap_in, &bitmap_out, sigma, nb_stddev);
	Py_END_ALLOW_THREADS;

	PyBuffer_Release(&img_in);
	PyBuffer_Release(&img_out);
	Py_RETURN_NONE;
}

#endif
