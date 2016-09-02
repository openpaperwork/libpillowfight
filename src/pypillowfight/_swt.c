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

#define _GNU_SOURCE // for qsort_r()

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

#define MAX_STROKE_WIDTH 256

#define DARK_ON_LIGHT 1

#define DEF_NB_POINTS_PER_RAY 32

#define PRECISION 0.25

#define IS_EDGE_LINE(val) ((val) > 0.0)

#define PRINT_TIME() printf("SWT: %d: %lds\n", __LINE__, time(NULL) - g_swt_start);

#ifdef PRINT_TIME
static time_t g_swt_start;
#endif

struct swt_point {
	int x;
	int y;
};

struct swt_ray {
	struct swt_ray *next;

	int nb_points;
	struct swt_point points[];
};

struct swt_output {
	struct pf_dbl_matrix swt; // The width of the strokes detected
	struct swt_ray *rays;
};

struct swt_gradient_matrixes {
	struct pf_dbl_matrix cos;
	struct pf_dbl_matrix sin;
};

static void init_output(struct swt_output *out)
{
	int x, y;

	for (x = 0 ; x < out->swt.size.x ; x++) {
		for (y = 0 ; y < out->swt.size.y ; y++) {
			PF_MATRIX_SET(&out->swt, x, y, DBL_MAX);
		}
	}
}

static void init_gradient(struct swt_gradient_matrixes *out, const struct pf_gradient_matrixes *in)
{
	int x, y;
	double val;

	out->cos = pf_dbl_matrix_new(in->direction.size.x, in->direction.size.y);
	out->sin = pf_dbl_matrix_new(in->direction.size.x, in->direction.size.y);

	for (x = 0 ; x < in->direction.size.x ; x++) {
		for (y = 0 ; y < in->direction.size.y ; y++) {
			val = PF_MATRIX_GET(&in->direction, x, y);
			PF_MATRIX_SET(&out->cos, x, y, cos(val));
			PF_MATRIX_SET(&out->sin, x, y, sin(val));
		}
	}
}

static void add_point_to_ray(const struct swt_point *current, void *callback_data) {
	struct swt_ray *ray = callback_data;
	ray->points[ray->nb_points] = *current;
	ray->nb_points++;
}

/*!
 * \brief follow the supposed stroke to its end and make sure the angles match
 *
 * \retval 0 if no match (--> no stroke)
 * \return number of points in the ray
 */
static inline int follow_stroke(
		const struct pf_dbl_matrix *edge,
		const struct pf_gradient_matrixes *pf_gradient,
		const struct swt_gradient_matrixes *swt_gradient,
		int x, int y,
		void (*on_ray_point_callback)(const struct swt_point *current, void *callback_data),
		void *callback_data)
{
	double g_x, g_y;
	double angle, angle_end;
	double cur_x, cur_y;
	struct swt_point current_pt;
	int nb_points = 0;

	g_x = PF_MATRIX_GET(&swt_gradient->cos, x, y);
	g_y = PF_MATRIX_GET(&swt_gradient->sin, x, y);
	if (DARK_ON_LIGHT) {
		g_x = -g_x;
		g_y = -g_y;
	}

	current_pt.x = x;
	current_pt.y = y;
	cur_x = 0.5 + x;
	cur_y = 0.5 + y;

	while(1) {
		// Move along the ray by incrementing with reduced values of (g_x, g_y)
		// TODO(Jflesch): Optim ?
		cur_x += g_x * PRECISION;
		cur_y += g_y * PRECISION;

		if ((floor(cur_x) == current_pt.x)
				&& (floor(cur_y) == current_pt.y)) {
			continue;
		}
		current_pt.x = floor(cur_x);
		current_pt.y = floor(cur_y);

		if (current_pt.x < 0
				|| current_pt.x >= edge->size.x
				|| current_pt.y < 0
				|| current_pt.y >= edge->size.y)
			break;

		nb_points++;
		if (nb_points >= MAX_STROKE_WIDTH) {
			return 0;
		}

		if (on_ray_point_callback)
			on_ray_point_callback(&current_pt, callback_data);

		if (IS_EDGE_LINE(PF_MATRIX_GET(edge, current_pt.x, current_pt.y))) {
			break;
		}
	}

	assert(current_pt.x != x || current_pt.y != y);

	angle = PF_MATRIX_GET(&pf_gradient->direction, x, y);
	angle_end = PF_MATRIX_GET(&pf_gradient->direction, current_pt.x, current_pt.y);
	if (DARK_ON_LIGHT)
		angle_end = -angle_end;
	angle += angle_end;
	if (angle >= (M_PI / 2.0) || angle <= -(M_PI / 2.0)) {
		// no stroke here
		return 0;
	}
	return nb_points;
}

static inline void find_stroke(struct swt_output *out,
		const struct pf_dbl_matrix *edge,
		const struct pf_gradient_matrixes *pf_gradient,
		const struct swt_gradient_matrixes *swt_gradient,
		int x, int y)
{
	int nb_points, i;
	struct swt_ray *ray;
	double val, length;

	nb_points = follow_stroke(edge, pf_gradient, swt_gradient, x, y, NULL, NULL);
	if (nb_points <= 0) {
		// no stroke here
		return;
	}

	// we found one --> fill in a ray structure
	ray = malloc(sizeof(struct swt_ray) + (sizeof(struct swt_point) * nb_points));
	ray->next = NULL;
	ray->nb_points = 0;
	nb_points = follow_stroke(edge, pf_gradient, swt_gradient, x, y, add_point_to_ray, ray);
	assert(nb_points > 0);
	assert(nb_points == ray->nb_points);

	// length of the stroke
	length = hypot(
		ray->points[0].x - ray->points[ray->nb_points - 1].x,
		ray->points[0].y - ray->points[ray->nb_points - 1].y
	);

	// add stroke's ray to the output
	ray->next = out->rays;
	out->rays = ray;

	// follow the ray: each point that has a current score bigger
	// than the length must have its score set to the length
	for (i = 0 ; i < nb_points ; i++) {
		val = PF_MATRIX_GET(&out->swt, ray->points[i].x, ray->points[i].y);
		val = MIN(val, length);
		PF_MATRIX_SET(&out->swt, ray->points[i].x, ray->points[i].y, val);
	}
}

static struct swt_output swt(const struct pf_dbl_matrix *edge, const struct pf_gradient_matrixes *pf_gradient)
{
	int x, y;
	struct swt_output out;
	struct swt_gradient_matrixes swt_gradient;

	assert(edge->size.x == pf_gradient->intensity.size.x);
	assert(edge->size.y == pf_gradient->intensity.size.y);
	assert(edge->size.x == pf_gradient->direction.size.x);
	assert(edge->size.y == pf_gradient->direction.size.y);

	memset(&out, 0, sizeof(out));
	out.swt = pf_dbl_matrix_new(edge->size.x, edge->size.y);
	init_output(&out);
	// pre-compute the cos() and sin() of each angle
	init_gradient(&swt_gradient, pf_gradient);

	for (x = 0 ; x < edge->size.x ; x++) {
		for (y = 0 ; y < edge->size.y ; y++) {

			if (IS_EDGE_LINE(PF_MATRIX_GET(edge, x, y))) {
				continue;
			}

			find_stroke(&out, edge, pf_gradient, &swt_gradient, x, y);

		}
	}

	pf_dbl_matrix_free(&swt_gradient.cos);
	pf_dbl_matrix_free(&swt_gradient.sin);

	return out;
}

static int compare_points(const void *_pt_a, const void *_pt_b, void *_swt_matrix) {
	const struct swt_point *pt_a = _pt_a;
	const struct swt_point *pt_b = _pt_b;
	const struct pf_dbl_matrix *swt_matrix = _swt_matrix;
	double val_a, val_b;

	val_a = PF_MATRIX_GET(swt_matrix, pt_a->x, pt_a->y);
	val_b = PF_MATRIX_GET(swt_matrix, pt_b->x, pt_b->y);
	if (val_a > val_b)
		return 1;
	if (val_a < val_b)
		return -1;
	return 0;
}

static void set_rays_down_to_ray_median(struct swt_output *rays)
{
	struct swt_ray *ray;
	int point_nb;
	double median, val;

	for (ray = rays->rays ; ray != NULL ; ray = ray->next) {
		assert(ray->nb_points > 0);

		qsort_r(ray->points, ray->nb_points, sizeof(ray->points[0]),
				compare_points, &rays->swt);

		if ((ray->nb_points % 2) == 0) {
			// we have an even number of points --> median is between
			// 2 points
			median = PF_MATRIX_GET(&rays->swt,
					ray->points[ray->nb_points / 2].x,
					ray->points[ray->nb_points / 2].y
				);
			median += PF_MATRIX_GET(&rays->swt,
					ray->points[(ray->nb_points / 2) - 1].x,
					ray->points[(ray->nb_points / 2) - 1].y
				);
			median /= 2.0;
		} else {
			median = PF_MATRIX_GET(&rays->swt,
					ray->points[ray->nb_points / 2].x,
					ray->points[ray->nb_points / 2].y
				);
		}

		for( point_nb = 0 ; point_nb < ray->nb_points ; point_nb++) {
			val = PF_MATRIX_GET(&rays->swt,
					ray->points[point_nb].x,
					ray->points[point_nb].y
				);
			val = MIN(val, median);
			PF_MATRIX_SET(&rays->swt,
					ray->points[point_nb].x,
					ray->points[point_nb].y,
					val
				);
		}
	}
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
	struct swt_ray *ray;
	int x, y;
	double val;

#ifdef PRINT_TIME
	g_swt_start = time(NULL);
#endif

	PRINT_TIME();

	in = pf_dbl_matrix_new(img_in->size.x, img_in->size.y);
	pf_rgb_bitmap_to_grayscale_dbl_matrix(img_in, &in);

	PRINT_TIME();

	// Compute edge & gradient
	edge = pf_canny_on_matrix(&in);

	PRINT_TIME();

	// Gaussian on the image
	out = pf_gaussian_on_matrix(&in, PF_GAUSSIAN_DEFAULT_SIGMA, PF_GAUSSIAN_DEFAULT_NB_STDDEV);
	// Find gradients
	// Jflesch> DetectText/TextDetection.cpp uses Scharr kernel instead of Sobel. Should we too ?
	PRINT_TIME();

	gradient = pf_sobel_on_matrix(&out);
	// Jflesch> DetectText/TextDetection.cpp apply a gaussian filter on the gradient
	// matrixes. Should we too ?
	pf_dbl_matrix_free(&in);
	pf_dbl_matrix_free(&out);

	PRINT_TIME();

	swt_out = swt(&edge, &gradient);
	PRINT_TIME();

	pf_dbl_matrix_free(&gradient.intensity);
	pf_dbl_matrix_free(&gradient.direction);
	pf_dbl_matrix_free(&edge);

	PRINT_TIME();

	set_rays_down_to_ray_median(&swt_out);

	// TODO: Find letter candidates
	// TODO: Filter letter candidates
	// TODO: Text line aggregation
	// TODO: Word detection
	// TODO: Mask

	PRINT_TIME();

	// temporary
	for (x = 0 ; x < swt_out.swt.size.x ; x++) {
		for (y = 0 ; y < swt_out.swt.size.y ; y++) {
			val = PF_MATRIX_GET(&swt_out.swt, x, y);
			if (val < 16.0) {
				val = PF_WHITE;
			} else {
				val = 0.0;
			}
			PF_MATRIX_SET(&swt_out.swt, x, y, val);
		}
	}
	pf_grayscale_dbl_matrix_to_rgb_bitmap(&swt_out.swt, img_out);

	PRINT_TIME();

	for (ray = swt_out.rays ; swt_out.rays != NULL ; ) {
		ray = swt_out.rays;
		swt_out.rays = ray->next;
		free(ray);
	}
	pf_dbl_matrix_free(&swt_out.swt);
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
