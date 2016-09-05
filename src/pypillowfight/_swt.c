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

#define DARK_ON_LIGHT 1
#define PRECISION 0.25
#define MAX_STROKE_WIDTH 256

#define IS_EDGE_LINE(val) ((val) > 0.0)

#define PRINT_TIME() printf("SWT: %d: %lds\n", __LINE__, time(NULL) - g_swt_start);
#define OUTPUT_INTERMEDIATE_IMGS 1

#ifdef PRINT_TIME
static time_t g_swt_start;
#endif

#ifdef OUTPUT_INTERMEDIATE_IMGS
#define DUMP_BITMAP(filename, bmp) pf_write_bitmap_to_ppm(filename ".ppm", bmp)
#define DUMP_MATRIX(filename, matrix, factor) pf_write_matrix_to_pgm(filename ".pgm", matrix, factor)
#else
#define DUMP_BITMAP(filename, bmp)
#define DUMP_MATRIX(filename, matrix, factor)
#endif

struct swt_point {
	int x;
	int y;
};

/* list of points (used to describe rays for instance) */
struct swt_points {
	struct swt_points *next;

	int nb_points;
	struct swt_point points[];
};

struct swt_output {
	struct pf_dbl_matrix swt; // The width of the strokes detected
	struct swt_points *rays;
};

struct swt_gradient_matrixes {
	struct pf_dbl_matrix cos;
	struct pf_dbl_matrix sin;
};

struct swt_adjacency {
	struct swt_point pt;
	int nb_neighboors;
	struct swt_adjacency *neighboors[8];
};

struct swt_adjacencies {
	struct {
		int x;
		int y;
	} size;
	struct swt_adjacency *adjacencies;
};

#define SWT_GET_ADJACENCY(adjs, a, b) (&((adjs)->adjacencies[(a) + ((b) * ((adjs)->size.x))]))

static void init_output(struct swt_output *out)
{
	int x, y;

	for (x = 0 ; x < out->swt.size.x ; x++) {
		for (y = 0 ; y < out->swt.size.y ; y++) {
			PF_MATRIX_SET(&out->swt, x, y, -1.0);
		}
	}
}

static inline struct swt_points *new_point_list(int capacity) {
	struct swt_points *pts;
	pts = calloc(1, sizeof(struct swt_points) + (sizeof(struct swt_point) * capacity));
	pts->next = NULL;
	pts->nb_points = 0;
	return pts;
}

static void add_point(const struct swt_point *current, void *callback_data) {
	struct swt_points *ray = callback_data;
	ray->points[ray->nb_points] = *current;
	ray->nb_points++;
}

static void free_rays(struct swt_points *rays)
{
	struct swt_points *ray, *nray;
	for (ray = rays, nray = (ray ? ray->next : NULL) ;
			ray != NULL ;
			ray = nray, nray = (nray ? nray->next : NULL)) {
		free(ray);
	}
}

static struct swt_adjacencies init_adjacencies(int x, int y)
{
	struct swt_adjacencies adjs;
	adjs.size.x = x;
	adjs.size.y = y;
	adjs.adjacencies = calloc(x * y, sizeof(struct swt_adjacency));
	return adjs;
}

#define add_adjacency(adj_pt, adj_neighboor) do { \
		assert((adj_pt)->nb_neighboors < 8); \
		(adj_pt)->neighboors[(adj_pt)->nb_neighboors] = (adj_neighboor); \
		(adj_pt)->nb_neighboors++; \
		assert((adj_neighboor)->nb_neighboors < 8); \
		(adj_neighboor)->neighboors[(adj_neighboor)->nb_neighboors] = (adj_pt); \
		(adj_neighboor)->nb_neighboors++; \
	} while(0)

static void free_adjacencies(struct swt_adjacencies *adjs)
{
	free(adjs->adjacencies);
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
	double fcur_x, fcur_y;
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

		fcur_x = floor(cur_x);
		fcur_y = floor(cur_y);

		if ((fcur_x == current_pt.x)
				&& (fcur_y == current_pt.y)) {
			continue;
		}

		if (fcur_x < 0
				|| fcur_x >= edge->size.x
				|| fcur_y < 0
				|| fcur_y >= edge->size.y)
			break;

		current_pt.x = floor(cur_x);
		current_pt.y = floor(cur_y);

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

	if (current_pt.x == x || current_pt.y == y) {
		// one pixel on the border of the image ==> not a stroke
		return 0;
	}

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
	struct swt_points *ray;
	double val, length;

	nb_points = follow_stroke(edge, pf_gradient, swt_gradient, x, y, NULL, NULL);
	if (nb_points <= 0) {
		// no stroke here
		return;
	}

	// we found one --> fill in a ray structure
	ray = new_point_list(nb_points);
	nb_points = follow_stroke(edge, pf_gradient, swt_gradient, x, y, add_point, ray);
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
		val = (val == -1.0 ? length : MIN(val, length));
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
	struct swt_points *ray;
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

static struct swt_adjacencies make_adjacencies_list(const struct pf_dbl_matrix *swt)
{
	const struct {
		int x;
		int y;
	} to_check[] = {
		{ .x = 1, .y = 0 }, // right
		{ .x = 1, .y = 1 }, // right down
		{ .x = 0, .y = 1 }, // down
		{ .x = -1, .y = 1 }, // left-down
	};
	struct swt_adjacencies adjs;
	struct swt_adjacency *adj_pt;
	struct swt_adjacency *adj_neighboor;
	int x, y, i;
	double val, val_neighboor;

	adjs = init_adjacencies(swt->size.x, swt->size.y);

	for (x = 0 ; x < adjs.size.x ; x++) {
		for (y = 0 ; y < adjs.size.y ; y++) {
			adj_pt = SWT_GET_ADJACENCY(&adjs, x, y);
			adj_pt->pt.x = x;
			adj_pt->pt.y = y;
		}
	}

	// fill in the adjacency list
	for (x = 0 ; x < adjs.size.x ; x++) {
		for (y = 0 ; y < adjs.size.y ; y++) {
			val = PF_MATRIX_GET(swt, x, y);
			if (val <= 0.0 || val >= DBL_MAX)
				continue;
			adj_pt = SWT_GET_ADJACENCY(&adjs, x, y);
			adj_pt->pt.x = x;
			adj_pt->pt.y = y;
			for (i = 0 ; i < PF_COUNT_OF(to_check) ; i++) {
				if ((x + to_check[i].x) < 0
						|| (x + to_check[i].x) >= adjs.size.x
						|| (y + to_check[i].y) < 0
						|| (y + to_check[i].y) >= adjs.size.y)
					continue;
				val_neighboor = PF_MATRIX_GET(swt, x + to_check[i].x, y + to_check[i].y);
				if (val_neighboor <= 0.0 || val_neighboor >= DBL_MAX
						|| (val / val_neighboor) >= 3.0 || (val_neighboor / val) >= 3.0)
					continue;
				adj_neighboor = SWT_GET_ADJACENCY(&adjs, x + to_check[i].x, y + to_check[i].y);
				add_adjacency(adj_pt, adj_neighboor);
			}
		}
	}

	return adjs;
}

struct adj_browsing_stack_element {
	struct swt_adjacency *adj;
	int adjacency_nb;
};

static int browse_adjacencies(struct swt_adjacencies *adjs,
		void (*callback)(int nb_group, int x, int y, void *callback_data),
		void *callback_data)
{
	struct pf_dbl_matrix visited; // OPTIM> Integers would be enough.
	int x, y, stack_depth, stack_offset;
	int nb_group = 0;
	struct adj_browsing_stack_element *stack;

	visited = pf_dbl_matrix_new(adjs->size.x, adjs->size.y);
	stack = malloc(sizeof(struct adj_browsing_stack_element) * adjs->size.x * adjs->size.y);

	for (x = 0 ; x < visited.size.x ; x++) {
		for (y = 0 ; y < visited.size.y ; y++) {
			if (PF_MATRIX_GET(&visited, x, y))
				continue;

			stack[0].adj = SWT_GET_ADJACENCY(adjs, x, y);
			if (stack[0].adj->nb_neighboors <= 0)
				continue;
			stack[0].adjacency_nb = 0;
			for (stack_depth = 0 ; stack_depth >= 0 ; ) {
				if (callback) {
					callback(nb_group, stack[stack_depth].adj->pt.x, stack[stack_depth].adj->pt.y, callback_data);
				}
				PF_MATRIX_SET(&visited, stack[stack_depth].adj->pt.x, stack[stack_depth].adj->pt.y, 1.0);
				stack_offset = -1; // go down in the stack by default
				for ( ; stack[stack_depth].adjacency_nb < stack[stack_depth].adj->nb_neighboors
						; stack[stack_depth].adjacency_nb++) {
					stack[stack_depth + 1].adj = stack[stack_depth].adj->neighboors[stack[stack_depth].adjacency_nb];
					if (!PF_MATRIX_GET(&visited, stack[stack_depth + 1].adj->pt.x, stack[stack_depth + 1].adj->pt.y)) {
						stack[stack_depth + 1].adjacency_nb = 0;
						stack[stack_depth].adjacency_nb++;
						stack_offset = 1; // go up in the stack
						break;
					}
				}
				stack_depth += stack_offset;
			}

			nb_group++;
		}
	}

	pf_dbl_matrix_free(&visited);

	return nb_group;
}

static void count_points(int nb_group, int x, int y, void *callback_data)
{
	int *nb_pts_per_group = callback_data;
	nb_pts_per_group[nb_group]++;
}

static void fillin_groups(int nb_group, int x, int y, void *callback_data)
{
	struct swt_points **groups = callback_data;
	struct swt_points *group = groups[nb_group];
	group->points[group->nb_points].x = x;
	group->points[group->nb_points].y = y;
	group->nb_points++;
}

/*!
 * Group all the points belonging to a same letter together, for all the letter.
 * Here we assume that adjacent points with similar stroke width belong to the
 * same letter.
 */
static struct swt_points **find_letters(const struct pf_dbl_matrix *swt)
{
	struct swt_adjacencies adjs;
	int nb_groups, i;
	int *nb_pts_per_group;
	struct swt_points **groups;

	PRINT_TIME();
	adjs = make_adjacencies_list(swt);
	PRINT_TIME();

	nb_groups = browse_adjacencies(&adjs, NULL, NULL);
	PRINT_TIME();

	fprintf(stderr, "SWT: %d groups found\n", nb_groups);

	nb_pts_per_group = calloc(nb_groups, sizeof(int));
	browse_adjacencies(&adjs, count_points, nb_pts_per_group);

	groups = calloc(nb_groups, sizeof(struct swt_points *));
	for (i = 0 ; i < nb_groups ; i++) {
		groups[i] = new_point_list(nb_pts_per_group[i]);
	}

	browse_adjacencies(&adjs, fillin_groups, groups);

	free(nb_pts_per_group);
	free_adjacencies(&adjs);

	return groups;
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
	struct swt_points **letters;
	int x, y;
	double val;
#ifdef OUTPUT_INTERMEDIATE_IMGS
	struct pf_dbl_matrix normalized;
#endif

#ifdef PRINT_TIME
	g_swt_start = time(NULL);
#endif

	PRINT_TIME();
	DUMP_BITMAP("swt_0000_input", img_in);

	in = pf_dbl_matrix_new(img_in->size.x, img_in->size.y);
	pf_rgb_bitmap_to_grayscale_dbl_matrix(img_in, &in);
	DUMP_MATRIX("swt_0001_grayscale", &in, 1.0);

	PRINT_TIME();

	// Compute edge & gradient
	edge = pf_canny_on_matrix(&in);
	DUMP_MATRIX("swt_0002_canny", &edge, 1.0);

	PRINT_TIME();

	// Gaussian on the image
	out = pf_gaussian_on_matrix(&in, 0.0, 5);
	DUMP_MATRIX("swt_0003_gaussian", &out, 1.0);
	PRINT_TIME();

	// Find gradients
	gradient = pf_sobel_on_matrix(&out, &g_pf_kernel_scharr_x, &g_pf_kernel_scharr_y, 0.0, 3);
	DUMP_MATRIX("swt_0004_sobel_intensity", &gradient.intensity, 1.0);
	DUMP_MATRIX("swt_0005_sobel_direction", &gradient.direction, 255.0 / 2.0 / M_PI);
	pf_dbl_matrix_free(&in);
	pf_dbl_matrix_free(&out);

	PRINT_TIME();

	swt_out = swt(&edge, &gradient);
	DUMP_MATRIX("swt_0006_swt", &swt_out.swt, 1.0);
	PRINT_TIME();

	pf_dbl_matrix_free(&gradient.intensity);
	pf_dbl_matrix_free(&gradient.direction);
	pf_dbl_matrix_free(&edge);

	PRINT_TIME();

	set_rays_down_to_ray_median(&swt_out);
	DUMP_MATRIX("swt_0007_ray_median", &swt_out.swt, 1.0);
	free_rays(swt_out.rays);

#ifdef OUTPUT_INTERMEDIATE_IMGS
	normalized = pf_normalize(&swt_out.swt, 0.0, 255.0);
	DUMP_MATRIX("swt_0008_normalized", &normalized, 1.0);
	pf_dbl_matrix_free(&normalized);
#endif

	PRINT_TIME();

	letters = find_letters(&swt_out.swt);

	// TODO: Find letter candidates (legally connected components)
	// TODO: Filter letter candidates
	// TODO: Text line aggregation
	// TODO: Word detection
	// TODO: Mask

	PRINT_TIME();

	// temporary
	for (x = 0 ; x < swt_out.swt.size.x ; x++) {
		for (y = 0 ; y < swt_out.swt.size.y ; y++) {
			val = PF_MATRIX_GET(&swt_out.swt, x, y);
			if (val > 1.0 && val <= 128.0) {
				val = 128.0;
			}
			PF_MATRIX_SET(&swt_out.swt, x, y, val);
		}
	}
	DUMP_MATRIX("swt_9999_out", &swt_out.swt, 1.0);
	pf_grayscale_dbl_matrix_to_rgb_bitmap(&swt_out.swt, img_out);

	PRINT_TIME();

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
