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
#include <stdlib.h>
#include <string.h>
#include <values.h>

#include <pillowfight/pillowfight.h>
#include <pillowfight/util.h>

#ifndef NO_PYTHON
#include "_pymod.h"
#endif

/*!
 * \brief Stroke Width Transform
 *
 * Ref: http://cmp.felk.cvut.cz/~cernyad2/TextCaptchaPdf/Detecting%20Text%20in%20Natural%20Scenes%20with%20Stroke%20Width%20Transform.pdf
 * Ref: https://github.com/aperrau/DetectText
 */

#define DARK_ON_LIGHT 1

#define DEFAULT_NB_POINTS_PER_RAY 16

struct swt_point {
	int x;
	int y;
};

struct swt_ray {
	struct swt_point *points;
	int nb_points;

	struct swt_point *beginning;
	struct swt_point *end;

	struct swt_ray *next;
};

struct swt_output {
	struct pf_dbl_matrix swt; // The width of the strokes detected
	struct swt_ray *rays;
};

struct swt_output swt(const struct pf_dbl_matrix *edge, const struct pf_gradient_matrixes *gradient)
{
	struct swt_output out;

	return out;
}

#ifndef NO_PYTHON
static
#endif
void pf_swt(const struct pf_bitmap *img_in, struct pf_bitmap *img_out)
{
	struct pf_dbl_matrix in, out;
	struct pf_gradient_matrixes gradient;
	struct pf_dbl_matrix edge;
	struct swt_output swt_out;

	in = pf_dbl_matrix_new(img_in->size.x, img_in->size.y);
	pf_rgb_bitmap_to_grayscale_dbl_matrix(img_in, &in);

	// Compute edge & gradient
	edge = pf_canny_on_matrix(&in);

	// Gaussian on the image
	out = pf_gaussian_on_matrix(&in, PF_GAUSSIAN_DEFAULT_SIGMA, PF_GAUSSIAN_DEFAULT_NB_STDDEV);
	// Find gradients
	// Jflesch> DetectText/TextDetection.cpp uses Scharr kernel instead of Sobel. Should we too ?
	gradient = pf_sobel_on_matrix(&out);
	// Jflesch> DetectText/TextDetection.cpp apply a gaussian filter on the gradient
	// matrixes. Should we too ?
	pf_dbl_matrix_free(&in);
	pf_dbl_matrix_free(&out);

	swt_out = swt(&edge, &gradient);
	pf_dbl_matrix_free(&gradient.intensity);
	pf_dbl_matrix_free(&gradient.direction);
	pf_dbl_matrix_free(&edge);

	// TODO: normalize ?
	// TODO: Find letter candidates
	// TODO: Filter letter candidates
	// TODO: Text line aggregation
	// TODO: Word detection
	// TODO: Mask

	//pf_grayscale_dbl_matrix_to_rgb_bitmap(XXX, img_out);
}

#ifndef NO_PYTHON

PyObject *pyswt(PyObject *self, PyObject* args)
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
	pf_swt(&bitmap_in, &bitmap_out);

	PyBuffer_Release(&img_in);
	PyBuffer_Release(&img_out);
	Py_RETURN_NONE;
}

#endif // !NO_PYTHON
