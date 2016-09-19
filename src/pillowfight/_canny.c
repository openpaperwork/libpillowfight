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

#ifndef NO_PYTHON
#include "_pymod.h"
#endif

#include <pillowfight/pillowfight.h>
#include <pillowfight/util.h>

/*!
 * \brief Canny edge detection
 *
 * Ref: "A computational Approach to Edge Detection" - John Canny
 * Ref: https://en.wikipedia.org/wiki/Canny_edge_detector
 */

#define LOW_THRESHOLD (0.686 * PF_WHITE)
#define HIGH_THRESHOLD (1.372 * PF_WHITE)

/*!
 * \brief Edge thinning
 *
 * Goes on each pixel. If one the neighbooring pixels that are on the direction
 * axis have a bigger intensity than the current one, the current one is killed
 * (intensity = 0.0)
 */
static void non_maximum_suppression(
		struct pf_dbl_matrix *intensity,
		const struct pf_dbl_matrix *direction
	)
{
	static const int axis_pt_coords[][2][2] = {
		{
			/*
			 * angle = ±0 + (45/2)
			 *   0 0 0
			 *   1 p 1
			 *   0 0 0
			 */
			{ 1, 0 },
			{ -1, 0 },
		},
		{
			/*
			 * angle = ±45° ± (45/2)
			 *   0 0 1
			 *   0 p 0
			 *   1 0 0
			 */
			{ 1, 1 },
			{ -1, -1 },
		},
		{
			/*
			 * angle = ±90° ± (45/2)
			 *   0 1 0
			 *   0 p 0
			 *   0 1 0
			 */
			{ 0, 1 },
			{ 0, -1 },
		},
		{
			/*
			 * angle = ±135° ± (45/2)
			 *   1 0 0
			 *   0 p 0
			 *   0 0 1
			 */
			{ -1, 1 },
			{ 1, -1 },
		},
	};

	int x, y, p, current_intensity, other_intensity;
	double angle;
	int axis_nb;

	assert(intensity->size.x == direction->size.x);
	assert(intensity->size.y == direction->size.y);

	for (x = 0 ; x < intensity->size.x ; x++) {
		for (y = 0 ; y < intensity->size.y ; y++) {

			angle = PF_MATRIX_GET(direction, x, y);
			current_intensity = PF_MATRIX_GET(intensity, x, y);

			/* we evaluate in 3 possible directions.
			 *
			 * To find quickly which axis we must evaluate,
			 * we bring the angle from [0-pi] to [0-4]
			 * and then ((round(angle)) % 4) to get the axis
			 */

			angle = angle * PF_COUNT_OF(axis_pt_coords) / M_PI;
			axis_nb = fmod(round(angle), PF_COUNT_OF(axis_pt_coords));

			for (p = 0 ; p < PF_COUNT_OF(axis_pt_coords[axis_nb]) ; p++) {
				if (x + axis_pt_coords[axis_nb][p][0] < 0
						|| x + axis_pt_coords[axis_nb][p][0] >= intensity->size.x
						|| y + axis_pt_coords[axis_nb][p][1] < 0
						|| y + axis_pt_coords[axis_nb][p][1] >= intensity->size.y)
					continue;
				other_intensity = PF_MATRIX_GET(intensity,
						x + axis_pt_coords[axis_nb][p][0],
						y + axis_pt_coords[axis_nb][p][1]);
				if (other_intensity > current_intensity) {
					// a neighbor is more intense than the current pixel
					// --> kill the current one
					PF_MATRIX_SET(intensity, x, y, 0.0);
					break;
				}
			}
		}
	}
}

static void apply_thresholds(struct pf_dbl_matrix *intensity) {
	int x;
	int y;
	double val;

	for (x = 0 ; x < intensity->size.x ; x++) {
		for (y = 0 ; y < intensity->size.y ; y++) {
			val = PF_MATRIX_GET(intensity, x, y);
			if (val > HIGH_THRESHOLD) {
				PF_MATRIX_SET(intensity, x, y, PF_WHITE);
			}
			else if (val <= LOW_THRESHOLD) {
				PF_MATRIX_SET(intensity, x, y, PF_BLACK);
			}
		}
	}
}

struct pf_dbl_matrix pf_canny_on_matrix(const struct pf_dbl_matrix *in)
{
	struct pf_dbl_matrix out;
	struct pf_gradient_matrixes out_gradient;

	// Remove details from the image to reduce filter sensitivity to crappy details
	out = pf_gaussian_on_matrix(in, 0.0, 3);

	// Compute the gradient intensity and direction
	out_gradient = pf_sobel_on_matrix(&out, PF_SOBEL_DEFAULT_KERNEL_X, PF_SOBEL_DEFAULT_KERNEL_Y, 0.0, 0);
	pf_dbl_matrix_free(&out);
	pf_dbl_matrix_free(&out_gradient.g_x);
	pf_dbl_matrix_free(&out_gradient.g_y);

	// Edge thinning
	non_maximum_suppression(&out_gradient.intensity, &out_gradient.direction);
	pf_dbl_matrix_free(&out_gradient.direction);

	// Apply thresholds to remove indecisive values
	apply_thresholds(&out_gradient.intensity);

	return out_gradient.intensity;
}

#ifndef NO_PYTHON
static
#endif
void pf_canny(const struct pf_bitmap *img_in, struct pf_bitmap *img_out)
{
	struct pf_dbl_matrix in, out;

	out = pf_dbl_matrix_new(img_in->size.x, img_in->size.y);
	pf_rgb_bitmap_to_grayscale_dbl_matrix(img_in, &out);

	in = out;
	out = pf_canny_on_matrix(&in);
	pf_dbl_matrix_free(&in);

	pf_grayscale_dbl_matrix_to_rgb_bitmap(&out, img_out);
	pf_dbl_matrix_free(&out);
}

#ifndef NO_PYTHON
PyObject *pycanny(PyObject *self, PyObject* args)
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

	Py_BEGIN_ALLOW_THREADS;
	pf_canny(&bitmap_in, &bitmap_out);
	Py_END_ALLOW_THREADS;

	PyBuffer_Release(&img_in);
	PyBuffer_Release(&img_out);
	Py_RETURN_NONE;
}

#endif
