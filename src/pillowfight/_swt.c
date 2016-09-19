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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <pillowfight/pillowfight.h>
#include <pillowfight/util.h>

#ifdef PF_WINDOWS
#include <windows.h>
#endif

#ifndef NO_PYTHON
#include "_pymod.h"
#endif

#define PRINT_TIME 0
#define OUTPUT_INTERMEDIATE_IMGS 0


/*!
 * \brief Stroke Width Transform
 *
 * Ref: http://cmp.felk.cvut.cz/~cernyad2/TextCaptchaPdf/Detecting%20Text%20in%20Natural%20Scenes%20with%20Stroke%20Width%20Transform.pdf
 * Ref: https://github.com/aperrau/DetectText
 */

#define DARK_ON_LIGHT 1
#define PRECISION 0.05

#define IS_EDGE_LINE(val) ((val) > 0.0)

#if PRINT_TIME == 1
#define PRINT_TIME_FN() printf("SWT: %d: %lds\n", __LINE__, time(NULL) - g_swt_start);
#else
#define PRINT_TIME_FN()
#endif


#if PRINT_TIME == 1
static time_t g_swt_start;
#else
#define PRINT_TIME_FN()
#endif

#if OUTPUT_INTERMEDIATE_IMGS == 1
#define DUMP_BITMAP(filename, bmp) pf_write_bitmap_to_ppm(filename ".ppm", bmp)
#define DUMP_MATRIX(filename, matrix, factor) pf_write_matrix_to_pgm(filename ".pgm", matrix, factor)
static int g_nb_rays;
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

struct swt_letter_stats {
	struct swt_point min;
	struct swt_point max;
#define SWT_STATS_DIMENSION(stats, axis) ((stats)->max.axis - (stats)->min.axis)
	struct {
		double r;
		double g;
		double b;
		double swt; // average stroke width
	} means;
	double variance;
	struct swt_point center;
	double median;
};

struct swt_letters { // .. or what we assume to be letters
	struct swt_points **letters;
	struct swt_letter_stats *stats;
	int nb_letters;
};

struct swt_link {
	const struct swt_points *letter;
	const struct swt_letter_stats *stats;

	struct swt_link *next;
};

struct swt_chain {
	struct swt_link *first;
	struct swt_link *last;

	double dist;
	struct {
		double x;
		double y;
	} direction;

	int merged;

	struct swt_chain *next;
};

struct swt_chains {
	struct swt_chain *first;
	struct swt_chain *last;
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

static __inline struct swt_points *new_point_list(int capacity) {
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
	struct swt_adjacency *adj_pt;

	adjs.size.x = x;
	adjs.size.y = y;
	adjs.adjacencies = calloc(x * y, sizeof(struct swt_adjacency));
	for (x = 0 ; x < adjs.size.x ; x++) {
		for (y = 0 ; y < adjs.size.y ; y++) {
			adj_pt = SWT_GET_ADJACENCY(&adjs, x, y);
			adj_pt->pt.x = x;
			adj_pt->pt.y = y;
		}
	}
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
static __inline int follow_stroke(
		const struct pf_dbl_matrix *edge,
		const struct pf_gradient_matrixes *pf_gradient,
		const struct swt_gradient_matrixes *swt_gradient,
		int x, int y,
		void (*on_ray_point_callback)(const struct swt_point *current, void *callback_data),
		void *callback_data)
{
	double g_x, g_y;
	double g_x_end, g_y_end;
	double cur_x, cur_y;
	double fcur_x, fcur_y;
	struct swt_point current_pt;
	int nb_points = 1;

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

	if (on_ray_point_callback)
		on_ray_point_callback(&current_pt, callback_data);

	while(1) {
		// Move along the ray by incrementing with reduced values of (g_x, g_y)
		// TODO(Jflesch): Optim ?
		cur_x += g_x * PRECISION;
		cur_y += g_y * PRECISION;

		fcur_x = floor(cur_x);
		fcur_y = floor(cur_y);

		if ((fcur_x == current_pt.x) && (fcur_y == current_pt.y)) {
			continue;
		}

		if (fcur_x < 0
				|| fcur_x >= edge->size.x
				|| fcur_y < 0
				|| fcur_y >= edge->size.y) {
			// if it touches the border of the image, it's not a valid ray
			return 0;
		}

		current_pt.x = fcur_x;
		current_pt.y = fcur_y;

		nb_points++;

		if (on_ray_point_callback)
			on_ray_point_callback(&current_pt, callback_data);

		if (IS_EDGE_LINE(PF_MATRIX_GET(edge, current_pt.x, current_pt.y))) {
			break;
		}

	}

	assert(current_pt.x != x || current_pt.y != y);

	g_x_end = PF_MATRIX_GET(&swt_gradient->cos, current_pt.x, current_pt.y);
	g_y_end = PF_MATRIX_GET(&swt_gradient->sin, current_pt.x, current_pt.y);
	if (DARK_ON_LIGHT) {
		g_x_end = -g_x_end;
		g_y_end = -g_y_end;
	}

	// check the angles are opposite to each other
	if (acos(g_x * -g_x_end + g_y * -g_y_end) >= M_PI / 2.0 ) {
		return 0;
	}

	return nb_points;
}

static __inline void find_stroke(struct swt_output *out,
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
#if OUTPUT_INTERMEDIATE_IMGS == 1
	g_nb_rays++;
#endif

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

	// pre-compute the cos() and sin() of each angle from the gradient
	init_gradient(&swt_gradient, pf_gradient);

	for (x = 0 ; x < edge->size.x ; x++) {
		for (y = 0 ; y < edge->size.y ; y++) {

			if (IS_EDGE_LINE(PF_MATRIX_GET(edge, x, y))) {
				find_stroke(&out, edge, pf_gradient, &swt_gradient, x, y);
			}

		}
	}

	pf_dbl_matrix_free(&swt_gradient.cos);
	pf_dbl_matrix_free(&swt_gradient.sin);

	return out;
}

#ifdef PF_WINDOWS
static int compare_points(void *_swt_matrix, const void *_pt_a, const void *_pt_b)
#else
static int compare_points(const void *_pt_a, const void *_pt_b, void *_swt_matrix)
#endif
    {
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

/*!
 * \warning modify the source array order !
 */
static double compute_points_median(const struct pf_dbl_matrix *swt,
		struct swt_points *pts)
{
	double median;

#ifdef PF_WINDOWS
    qsort_s
#else
	qsort_r
#endif
	(pts->points, pts->nb_points, sizeof(pts->points[0]),
		compare_points, (void*)swt);

	if ((pts->nb_points % 2) == 0) {
		// we have an even number of points --> median is between
		// 2 points
		median = PF_MATRIX_GET(swt,
				pts->points[pts->nb_points / 2].x,
				pts->points[pts->nb_points / 2].y
			);
		median += PF_MATRIX_GET(swt,
				pts->points[(pts->nb_points / 2) - 1].x,
				pts->points[(pts->nb_points / 2) - 1].y
			);
		median /= 2.0;
	} else {
		median = PF_MATRIX_GET(swt,
				pts->points[pts->nb_points / 2].x,
				pts->points[pts->nb_points / 2].y
			);
	}

	return median;
}

static void set_rays_down_to_ray_median(struct swt_output *rays)
{
	struct swt_points *ray;
	int point_nb;
	double median, val;

	for (ray = rays->rays ; ray != NULL ; ray = ray->next) {
		assert(ray->nb_points > 0);

		median = compute_points_median(&rays->swt, ray);

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
		{ 1, 0 }, // right
		{ 1, 1 }, // right down
		{ 0, 1 }, // down
		{ -1, 1 }, // left-down
	};
	struct swt_adjacencies adjs;
	struct swt_adjacency *adj_pt;
	struct swt_adjacency *adj_neighboor;
	int x, y, i;
	double val, val_neighboor;

	adjs = init_adjacencies(swt->size.x, swt->size.y);

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
static struct swt_letters find_possible_letters(const struct pf_dbl_matrix *swt)
{
	struct swt_adjacencies adjs;
	int i;
	int *nb_pts_per_group;
	struct swt_letters out;

	PRINT_TIME_FN();
	adjs = make_adjacencies_list(swt);
	PRINT_TIME_FN();

	out.nb_letters = browse_adjacencies(&adjs, NULL, NULL);
	PRINT_TIME_FN();

	nb_pts_per_group = calloc(out.nb_letters, sizeof(int));
	browse_adjacencies(&adjs, count_points, nb_pts_per_group);

	out.letters = calloc(out.nb_letters, sizeof(struct swt_points *));
	out.stats = calloc(out.nb_letters, sizeof(struct swt_letter_stats));
	for (i = 0 ; i < out.nb_letters ; i++) {
		out.letters[i] = new_point_list(nb_pts_per_group[i]);
	}

	browse_adjacencies(&adjs, fillin_groups, out.letters);

	free(nb_pts_per_group);
	free_adjacencies(&adjs);

	return out;
}

static void compute_letter_stats(
		const struct pf_bitmap *bitmap,
		const struct pf_dbl_matrix *swt,
		struct swt_points *points,
		struct swt_letter_stats *stats)
{
	int i;
	double val;
	const struct swt_point *pt;
	union pf_pixel pixel;

	assert(points->nb_points > 0);

	stats->min.x = INT_MAX;
	stats->min.y = INT_MAX;
	stats->max.x = 0;
	stats->max.y = 0;
	stats->means.r = 0.0;
	stats->means.g = 0.0;
	stats->means.g = 0.0;
	stats->means.swt = 0.0;
	stats->variance = 0.0;
	stats->median = 0.0;

	for (i = 0 ; i < points->nb_points ; i++) {
		pt = &points->points[i];
		pixel = PF_GET_PIXEL(bitmap, pt->x, pt->y);
		val = PF_MATRIX_GET(swt, pt->x, pt->y);
		stats->means.swt += val;
		stats->means.r += pixel.color.r;
		stats->means.g += pixel.color.g;
		stats->means.b += pixel.color.b;
		stats->min.x = MIN(stats->min.x, pt->x);
		stats->min.y = MIN(stats->min.y, pt->y);
		stats->max.x = MAX(stats->max.x, pt->x);
		stats->max.y = MAX(stats->max.y, pt->y);
	}
	stats->means.swt /= points->nb_points;
	stats->means.r /= points->nb_points;
	stats->means.g /= points->nb_points;
	stats->means.b /= points->nb_points;

	for (i = 0 ; i < points->nb_points ; i++) {
		pt = &points->points[i];
		val = PF_MATRIX_GET(swt, pt->x, pt->y);
		stats->variance += (val - stats->means.swt) * (val - stats->means.swt);
	}
	stats->variance /= points->nb_points;

	stats->center.x = (stats->max.x + stats->min.x) / 2;
	stats->center.y = (stats->max.y + stats->min.y) / 2;

	stats->median = compute_points_median(swt, points);
}

static int check_ratio(
		const struct swt_points *points,
		struct swt_letter_stats *stats)
{
#define ROTATED_BOUNDING_BOX_INCREMENT (M_PI/36)
#define MIN_ASPECT_RATIO (1./10.)
#define MAX_ASPECT_RATIO (10.)

	double min_x, max_x;
	double min_y, max_y;
	double theta;
	int i;
	const struct swt_point *pt;
	double x, y;
	double w, h;
	double ratio;

	for (theta = ROTATED_BOUNDING_BOX_INCREMENT ;
			theta < M_PI / 2.0 ;
			theta += ROTATED_BOUNDING_BOX_INCREMENT) {

		min_x = DBL_MAX;
		min_y = DBL_MAX;
		max_x = -DBL_MAX;
		max_y = -DBL_MAX;

		for (i = 0 ; i < points->nb_points ; i++) {
			pt = &points->points[i];
			x = (pt->x * cos(theta)) + (pt->y * -sin(theta));
			y = (pt->x * sin(theta)) + (pt->y * cos(theta));
			min_x = MIN(min_x, x);
			max_x = MAX(max_x, x);
			min_y = MIN(min_y, y);
			max_y = MAX(max_y, y);
		}
		w = max_x - min_x;
		h = max_y - min_y;
		ratio = (w / h);

		// check if the aspect ratio is between 1/10 and 10
		if (ratio >= MIN_ASPECT_RATIO && ratio <= MAX_ASPECT_RATIO) {
			return 1;
		}
	}
	return 0;
}

static void compute_all_letter_stats(
		const struct pf_bitmap *bitmap,
		const struct pf_dbl_matrix *swt,
		const struct swt_letters *letters
	)
{
	int i;

	for (i = 0 ; i < letters->nb_letters ; i++) {
		compute_letter_stats(bitmap, swt, letters->letters[i], &letters->stats[i]);
	}
}

static int is_valid_letter(const struct pf_dbl_matrix *swt,
		struct swt_points *points,
		struct swt_letter_stats *stats)
{
#define MAX_Y_RATIO 0.33 // ratio to the image size
#define MAX_VARIANCE 2.0

	if (stats->variance > MAX_VARIANCE * stats->means.swt)
		return 0;
	// Jflesch> Assumption: Writing is horizontal
	if (((double)SWT_STATS_DIMENSION(stats, y)) / swt->size.y > MAX_Y_RATIO)
		return 0;
	if (!check_ratio(points, stats)) {
		return 0;
	}

	return 1;
}

static struct swt_letters filter_possible_letters(const struct pf_dbl_matrix *swt,
		const struct swt_letters *possible_letters)
{
	struct swt_letters out;
	int x;

	out.nb_letters = 0;
	out.letters = calloc(possible_letters->nb_letters, sizeof(struct swt_points *));
	out.stats = calloc(possible_letters->nb_letters, sizeof(struct swt_letter_stats));

	for (x = 0 ; x < possible_letters->nb_letters ; x++) {
		if (is_valid_letter(swt,
				possible_letters->letters[x],
				&possible_letters->stats[x])) {
			out.letters[out.nb_letters] = possible_letters->letters[x];
			memcpy(&out.stats[out.nb_letters], &possible_letters->stats[x],
				sizeof(out.stats[out.nb_letters]));
			out.nb_letters++;
		}
	}

	// resize down
	out.letters = realloc(
			out.letters,
			out.nb_letters * sizeof(struct swt_points *)
		);
	out.stats = realloc(
			out.stats,
			out.nb_letters * sizeof(struct swt_letter_stats)
		);

	return out;
}

static struct swt_letters filter_letters_by_center(const struct swt_letters *possible_letters)
{
	struct swt_letters out;
	int i, j, count;
	struct swt_letter_stats *stats_i, *stats_j;

#define MAX_COUNT 3

	out.nb_letters = 0;
	out.letters = calloc(possible_letters->nb_letters, sizeof(struct swt_points *));
	out.stats = calloc(possible_letters->nb_letters, sizeof(struct swt_letter_stats));

	for (i = 0 ; i < possible_letters->nb_letters ; i++) {
		count = 0;
		stats_i = &possible_letters->stats[i];
		for (j = 0 ; j < possible_letters->nb_letters ; j++) {
			if (i == j)
				continue;
			stats_j = &possible_letters->stats[j];

			if (stats_i->min.x <= stats_j->center.x
					&& stats_j->center.x <= stats_i->max.x
					&& stats_i->min.y <= stats_j->center.y
					&& stats_j->center.y <= stats_i->max.y) {
				count++;
			}
		}

		if (count < 3) {
			out.letters[out.nb_letters] = possible_letters->letters[i];
			memcpy(&out.stats[out.nb_letters], &possible_letters->stats[i],
				sizeof(out.stats[out.nb_letters]));
			out.nb_letters++;
		}
	}

	// resize down
	out.letters = realloc(
			out.letters,
			out.nb_letters * sizeof(struct swt_points *)
		);
	out.stats = realloc(
			out.stats,
			out.nb_letters * sizeof(struct swt_letter_stats)
		);
	return out;
}

#if OUTPUT_INTERMEDIATE_IMGS == 1
static void dump_letters(const char *filepath, const struct pf_bitmap *img_in,
		const struct swt_letters *letters)
{
	struct pf_bitmap img_out = {
		.size = {
			.x = img_in->size.x,
			.y = img_in->size.y,
		},
		.pixels = malloc(img_in->size.x * img_in->size.y * sizeof(union pf_pixel)),
	};
	int i, j;
	const struct swt_points *letter;
	const struct swt_point *pt;
	uint32_t pixel;

	// init with default color
	for (i = 0 ; i < img_in->size.x ; i++) {
		for (j = 0 ; j < img_in->size.y ; j++) {
			PF_SET_PIXEL(&img_out, i, j, 0xFF305030 /* ABGR */);
		}
	}

	// fill in with the original image only where letters
	// have been detected
	for (i = 0 ; i < letters->nb_letters ; i++) {
		letter = letters->letters[i];
		for (j = 0 ; j < letter->nb_points ; j++) {
			pt = &letter->points[j];
			pixel = PF_GET_PIXEL(img_in, pt->x, pt->y).whole;
			PF_SET_PIXEL(&img_out, pt->x, pt->y, pixel);
		}
	}
	pf_write_bitmap_to_ppm(filepath, &img_out);
	free(img_out.pixels);
}
#endif

static struct swt_chains make_valid_pairs(const struct swt_letters *letters)
{
#define MAX_MEDIAN_RATIO 2.0
#define MAX_DIMENSION_RATIO 2.0
#define MAX_COLOR_DIST 1600.0
#define MAX_DIST_RATIO 9.0

	struct swt_chains chains = { NULL };
	int nb_pairs = 0;
	int letter_idx_a, letter_idx_b;
	const struct swt_points *l_a;
	const struct swt_letter_stats *l_stats_a;
	const struct swt_points *l_b;
	const struct swt_letter_stats *l_stats_b;
	struct swt_link *link_a, *link_b;
	struct swt_chain *chain;
	double val_a, val_b;
	double dist, color_dist;
	double weird;
	double h;

	for (letter_idx_a = 0 ;
			letter_idx_a < letters->nb_letters ;
			letter_idx_a++) {
		l_a = letters->letters[letter_idx_a];
		l_stats_a = &letters->stats[letter_idx_a];

		for (letter_idx_b = letter_idx_a + 1 ;
				letter_idx_b < letters->nb_letters ;
				letter_idx_b++) {
			l_b = letters->letters[letter_idx_b];
			l_stats_b = &letters->stats[letter_idx_b];

			// ignore pairs that obviously can't be valid

			val_a = l_stats_a->median;
			val_b = l_stats_b->median;
			// Jflesch> DetectText checks that "(a/b) <= max || (b/a) <= max"
			// but it is always true !? I think they wanted to test
			// "(a/b) <= max && (b/a) <= max" as done here.
			if ((val_a / val_b > MAX_MEDIAN_RATIO) || (val_b / val_a > MAX_MEDIAN_RATIO))
				continue;

			// Jflesch> Assumption: Writing is horizontal
			val_a = SWT_STATS_DIMENSION(l_stats_a, y);
			val_b = SWT_STATS_DIMENSION(l_stats_b, y);
			// Jflesch> DetectText checks that  "(a/b) <= max || (b/a) <= max"
			// but it is always true !? I think they wanted to test
			// "(a/b) <= max && (b/a) <= max" as done here.
			if ((val_a / val_b > MAX_DIMENSION_RATIO) || (val_b / val_a > MAX_DIMENSION_RATIO))
				continue;

			// compute the color distance, but don't bother to sqrt() it.
			color_dist = (
					(
					 (l_stats_a->means.r - l_stats_b->means.r)
					 * (l_stats_a->means.r - l_stats_b->means.r)
					)
					+
					(
					 (l_stats_a->means.g - l_stats_b->means.g)
					 * (l_stats_a->means.g - l_stats_b->means.g)
					)
					+
					(
					 (l_stats_a->means.b - l_stats_b->means.b)
					 * (l_stats_a->means.b - l_stats_b->means.b)
					)
				     );
			if (color_dist >= MAX_COLOR_DIST)
				continue;

			// compute the distance from each centers, but don't bother sqrt() it.
			dist = (
					(
					 (l_stats_a->center.x - l_stats_b->center.x)
					 * (l_stats_a->center.x - l_stats_b->center.x)
					)
					+
					(
					 (l_stats_a->center.y - l_stats_b->center.y)
					 * (l_stats_a->center.y - l_stats_b->center.y)
					)
			       );

			weird = MAX(
					MIN(
						SWT_STATS_DIMENSION(l_stats_a, x),
						SWT_STATS_DIMENSION(l_stats_b, y)
					   ),
					MIN(
						SWT_STATS_DIMENSION(l_stats_b, x),
						SWT_STATS_DIMENSION(l_stats_a, y)
					   )
				   );
			weird *= weird;

			if (dist >= MAX_DIST_RATIO * weird)
				continue;

			chain = calloc(1, sizeof(struct swt_chain));
			link_a = calloc(1, sizeof(struct swt_link));
			link_b = calloc(1, sizeof(struct swt_link));

			link_a->letter = l_a;
			link_a->stats = l_stats_a;
			link_b->letter = l_b;
			link_b->stats = l_stats_b;
			link_a->next = link_b;

			chain->first = link_a;
			chain->last = link_b;
			chain->dist = dist;

			val_a = l_stats_a->center.x - l_stats_b->center.x;
			val_b = l_stats_a->center.y - l_stats_b->center.y;
			h = hypot(val_a, val_b);
			val_a /= h;
			val_b /= h;
			chain->direction.x = val_a;
			chain->direction.y = val_b;

			chain->next = NULL;

			if (chains.first == NULL)
				chains.first = chain;
			else
				chains.last->next = chain;
			chains.last = chain;

			nb_pairs++;
		}
	}

#if OUTPUT_INTERMEDIATE_IMGS == 1
	fprintf(stderr, "SWT> %d valid pairs found\n", nb_pairs);
#endif

	return chains;
}

#ifdef PF_WINDOWS
static int compare_chains(void *_nop, const void *_chain_a, const void *_chain_b)
#else
static int compare_chains(const void *_chain_a, const void *_chain_b, void *_nop)
#endif
{
	struct swt_chain * const *chain_a = _chain_a;
	struct swt_chain * const *chain_b = _chain_b;

	if ((*chain_a)->dist > (*chain_b)->dist)
		return 1;
	if ((*chain_a)->dist < (*chain_b)->dist)
		return -1;
	return 0;
}

static void reverse_chain(struct swt_chain *chain)
{
	struct swt_link *link, *plink, *nlink;

	for (plink = NULL,
			link = chain->first,
			nlink = (link ? link->next : NULL) ;

			link != NULL ;

			plink = link,
			link = nlink,
			nlink = (link ? link->next : NULL)) {
		link->next = plink;
	}

	link = chain->first;
	chain->first = chain->last;
	chain->last = link;
}

static int merge_chains(struct swt_chains *in_chains)
{
	int nb_chains, nb_after;
	struct swt_chain *chain_i, *chain_j, *t_chain;
	struct swt_chain **chains;
	int i, j;
	double i_dir_x, i_dir_y, j_dir_x, j_dir_y;
	double h;

#define MERGE_STRICTNESS (M_PI / 6.0)
#define CHAINS_SHARE_ONE_END(chain_a, chain_b) \
	((chain_a)->first->letter == (chain_b)->first->letter \
	 || (chain_a)->first->letter == (chain_b)->last->letter \
	 || (chain_a)->last->letter == (chain_b)->last->letter \
	 || (chain_a)->last->letter == (chain_b)->first->letter)

	// rearrange the list as an array so we can sort it
	nb_chains = 0;
	for (chain_i = in_chains->first; chain_i != NULL ; chain_i = chain_i->next)
		nb_chains++;
	chains = calloc(nb_chains, sizeof(struct swt_chain *));
	for (i = 0, chain_i = in_chains->first;
			chain_i != NULL ;
			chain_i = chain_i->next, i++) {
		chain_i->merged = 0;
		chains[i] = chain_i;
	}

	// sort the chains by distance
#ifdef PF_WINDOWS
	qsort_s
#else
    qsort_r
#endif
	(chains, nb_chains, sizeof(struct swt_chain *), compare_chains, NULL);

	// merge what can be merged
	for (i = 0 ; i < nb_chains ; i++) {
		chain_i = chains[i];
		assert(chain_i->first);
		assert(chain_i->last);
		for (j = 0 ; j < nb_chains ; j++) {
			if (i == j)
				continue;

			chain_j = chains[j];
			assert(chain_j->first);
			assert(chain_j->last);

			if (chain_i->merged || chain_j->merged)
				continue;
			if (!CHAINS_SHARE_ONE_END(chain_i, chain_j))
				continue;

			// can merge ?
			i_dir_x = chain_i->direction.x;
			j_dir_x = chain_j->direction.x;
			i_dir_y = chain_i->direction.y;
			j_dir_y = chain_j->direction.y;
			if (chain_i->first == chain_j->first
					|| chain_i->last == chain_j->last) {
				j_dir_x *= -1;
				j_dir_y *= -1;
			}
			if (acos((i_dir_x * j_dir_x) + (i_dir_y * j_dir_y)) >= MERGE_STRICTNESS)
				continue;

			// do the merges
			if (chain_i->first->letter == chain_j->first->letter) {
				// the 2 chains start with the same element
				// we use the end of one of them as start,
				// and the end of the other as end

				// reverse j
				reverse_chain(chain_j);
				// now chains[i]->first == chains[j]->last
			} else if (chain_i->last->letter == chain_j->last->letter) {
				// the 2 chains ends with the same element

				// reverse j
				reverse_chain(chain_j);
				// now chains[i]->last == chains[j]->first
			}

			if (chain_i->last->letter == chain_j->first->letter) {
				// flips the pointers for convenience
				// now: chain_i->first == chain_j->last
				t_chain = chain_i;
				chain_i = chain_j;
				chain_j = t_chain;
			}

			assert(chain_i->first->letter == chain_j->last->letter);

			// merge
			// make j followed by i (free the redundant letter link)
			chain_j->last->next = chain_i->first->next;
			free(chain_i->first);
			// make i start where j starts
			chain_i->first = chain_j->first;
			chain_j->merged = 1; // do not use again

			// recompute the chain stats

			i_dir_x = (chain_i->first->stats->center.x
					- chain_i->last->stats->center.x);
			i_dir_y = (chain_i->first->stats->center.y
					- chain_i->last->stats->center.y);

			chain_i->dist = (i_dir_x * i_dir_x) + (i_dir_y * i_dir_y);
			h = hypot(i_dir_x, i_dir_y);

			i_dir_x /= h;
			i_dir_y /= h;

			chain_i->direction.x = i_dir_x;
			chain_i->direction.y = i_dir_y;
		}
	}

	// rebuild the linked list
	in_chains->first = NULL;
	in_chains->last = NULL;

	chain_i = NULL; // previous chain
	nb_after = 0;
	for (j = 0 ; j < nb_chains ; j++) {
		chain_j = chains[j];
		if (chain_j->merged) {
			free(chain_j);
			continue;
		}
		chain_j->next = NULL;
		nb_after++;
		if (in_chains->first == NULL)
			in_chains->first = chain_j;
		in_chains->last = chain_j;
		if (chain_i)
			chain_i->next = chain_j;
		chain_i = chain_j;
	}

	free(chains);

	return nb_after;
}

static __inline int get_nb_links(const struct swt_chain *chain)
{
	const struct swt_link *link;
	int nb;

	nb = 0;
	for (link = chain->first ; link != NULL ; link = link->next) {
		nb++;
	}
	return nb;
}

static int render_chains_boxes(
		const struct pf_dbl_matrix *swt,
		const struct pf_bitmap *in,
		struct pf_bitmap *out,
		const struct swt_chains *chains,
		enum pf_swt_output output_type
	)
{
	const struct swt_chain *chain;
	const struct swt_link *link;
	const struct swt_letter_stats *stats;
	int nb_letters = 0;
	uint32_t pixel;
	int min_x, max_x, min_y, max_y;
	int x, y;

#define MIN_COMPONENTS 3

	assert(output_type == PF_SWT_OUTPUT_ORIGINAL_BOXES);

	// put default color everywhere
	memset(
			out->pixels, PF_WHOLE_WHITE,
			out->size.x * out->size.y * sizeof(union pf_pixel)
	      );

	for (chain = chains->first ; chain != NULL ; chain = chain->next) {
		// ignore chains with too few letters components
		if (get_nb_links(chain) < MIN_COMPONENTS)
			continue;

		min_x = INT_MAX;
		max_x = -INT_MAX;
		min_y = INT_MAX;
		max_y = -INT_MAX;

		for (link = chain->first ; link != NULL ; link = link->next) {
			stats = link->stats;
			min_x = MIN(min_x, stats->min.x);
			max_x = MAX(max_x, stats->max.x);
			min_y = MIN(min_y, stats->min.y);
			max_y = MAX(max_y, stats->max.y);
			nb_letters++;
		}

		for (x = min_x ; x < max_x ; x++) {
			for (y = min_y ; y < max_y ; y++) {
				pixel = PF_GET_PIXEL(in, x, y).whole;
				PF_SET_PIXEL(out, x, y, pixel);
			}
		}
	}

	return nb_letters;
}

static int render_chains_text(
		const struct pf_dbl_matrix *swt,
		const struct pf_bitmap *in,
		struct pf_bitmap *out,
		const struct swt_chains *chains,
		enum pf_swt_output output_type
	)
{
	const struct swt_chain *chain;
	const struct swt_link *link;
	const struct swt_points *letter;
	const struct swt_point *pt;
	struct pf_dbl_matrix out_val;
	struct pf_dbl_matrix grayscale;
	int nb_letters = 0;
	int x;
	double val;

#define MIN_COMPONENTS 3

	assert(output_type == PF_SWT_OUTPUT_BW_TEXT
			|| output_type == PF_SWT_OUTPUT_GRAYSCALE_TEXT);

	out_val = pf_dbl_matrix_new(in->size.x, in->size.y);

	for (chain = chains->first ; chain != NULL ; chain = chain->next) {
		// ignore chains with too few letters components
		if (get_nb_links(chain) < MIN_COMPONENTS)
			continue;

		for (link = chain->first ; link != NULL ; link = link->next) {
			letter = link->letter;
			nb_letters++;

			for (x = 0 ; x < letter->nb_points ; x++) {
				pt = &letter->points[x];
				val = PF_MATRIX_GET(swt, pt->x, pt->y);
				if (output_type == PF_SWT_OUTPUT_BW_TEXT) {
					val = (val ? PF_WHITE : PF_BLACK);
				}
				PF_MATRIX_SET(&out_val, pt->x, pt->y, val);
			}
		}
	}

	/* normalize and reverse colors */
	grayscale = pf_normalize(&out_val, 0.0, 255.0, 0.0);
	pf_dbl_matrix_free(&out_val);

	pf_matrix_to_rgb_bitmap(&grayscale, out, COLOR_R);
	pf_matrix_to_rgb_bitmap(&grayscale, out, COLOR_G);
	pf_matrix_to_rgb_bitmap(&grayscale, out, COLOR_B);

	pf_dbl_matrix_free(&grayscale);

	return nb_letters;
}


#ifndef NO_PYTHON
static
#endif
void pf_swt(const struct pf_bitmap *img_in, struct pf_bitmap *img_out,
		enum pf_swt_output output_type)
{
	struct pf_dbl_matrix in, out;
	struct pf_gradient_matrixes gradient;
	struct pf_dbl_matrix edge;
	struct swt_output swt_out;
	struct swt_letters all_possible_letters;
	struct swt_letters possible_letters;
	struct swt_letters letters;
	struct swt_chains chains;
	struct swt_chain *chain, *nchain;
	struct swt_link *link, *nlink;
	int x;
#if OUTPUT_INTERMEDIATE_IMGS == 1
	int y;
	double val, max;
	struct pf_dbl_matrix normalized_a, normalized_b, reversed;
#endif

#if PRINT_TIME == 1
	g_swt_start = time(NULL);
#endif

	PRINT_TIME_FN();
	DUMP_BITMAP("swt_0000_input", img_in);

	in = pf_dbl_matrix_new(img_in->size.x, img_in->size.y);
	pf_rgb_bitmap_to_grayscale_dbl_matrix(img_in, &in);
	DUMP_MATRIX("swt_0001_grayscale", &in, 1.0);

	PRINT_TIME_FN();

	// Compute edge & gradient
	edge = pf_canny_on_matrix(&in);
	DUMP_MATRIX("swt_0002_canny", &edge, 1.0);

	PRINT_TIME_FN();

	// Gaussian on the image
	out = pf_gaussian_on_matrix(&in, 0.0, 3);
	DUMP_MATRIX("swt_0003_gaussian", &out, 1.0);
	PRINT_TIME_FN();

	// Find gradients
	gradient = pf_sobel_on_matrix(&out, &g_pf_kernel_scharr_x, &g_pf_kernel_scharr_y, 0.0, 0);
	DUMP_MATRIX("swt_0004_sobel_intensity", &gradient.intensity, 1.0);
	DUMP_MATRIX("swt_0005_sobel_direction", &gradient.direction, 255.0 / 2.0 / M_PI);
	pf_dbl_matrix_free(&out);

	pf_dbl_matrix_free(&in);

	PRINT_TIME_FN();

#if OUTPUT_INTERMEDIATE_IMGS == 1
	g_nb_rays = 0;
#endif

	swt_out = swt(&edge, &gradient);
	DUMP_MATRIX("swt_0006_swt", &swt_out.swt, 1.0);
	PRINT_TIME_FN();

#if OUTPUT_INTERMEDIATE_IMGS == 1
	fprintf(stderr, "SWT> %d rays found\n", g_nb_rays);
#endif

	pf_dbl_matrix_free(&gradient.intensity);
	pf_dbl_matrix_free(&gradient.direction);
	pf_dbl_matrix_free(&gradient.g_x);
	pf_dbl_matrix_free(&gradient.g_y);
	pf_dbl_matrix_free(&edge);

	PRINT_TIME_FN();

	set_rays_down_to_ray_median(&swt_out);
	DUMP_MATRIX("swt_0007_ray_median", &swt_out.swt, 1.0);
	free_rays(swt_out.rays);

#if OUTPUT_INTERMEDIATE_IMGS == 1
	max = 0.0;
	for (x = 0; x < swt_out.swt.size.x ; x++) {
		for (y = 0 ; y < swt_out.swt.size.y ; y++) {
			val = PF_MATRIX_GET(&swt_out.swt, x, y);
			max = MAX(max, val);
		}
	}
	normalized_a = pf_dbl_matrix_new(swt_out.swt.size.x, swt_out.swt.size.y);
	for (x = 0; x < swt_out.swt.size.x ; x++) {
		for (y = 0 ; y < swt_out.swt.size.y ; y++) {
			val = PF_MATRIX_GET(&swt_out.swt, x, y);
			if (val < 0)
				val = max;
			PF_MATRIX_SET(&normalized_a, x, y, val);
		}
	}
	normalized_b = pf_normalize(&normalized_a, 0.0, 0.0, 255.0);
	pf_dbl_matrix_free(&normalized_a);
	// reverse it to make the short rays more visible
	// and the bigger ones less visible
	reversed = pf_grayscale_reverse(&normalized_b);
	pf_dbl_matrix_free(&normalized_b);
	DUMP_MATRIX("swt_0008_normalized", &reversed, 1.0);
	pf_dbl_matrix_free(&reversed);
#endif

	PRINT_TIME_FN();

	all_possible_letters = possible_letters = find_possible_letters(&swt_out.swt);
#if OUTPUT_INTERMEDIATE_IMGS == 1
	fprintf(stderr, "SWT> %d possible letters found\n", possible_letters.nb_letters);
	dump_letters("swt_0009_all_possible_letters.ppm", img_in, &possible_letters);
#endif

	compute_all_letter_stats(img_in, &swt_out.swt, &all_possible_letters);

	letters = filter_possible_letters(&swt_out.swt, &possible_letters);
#if OUTPUT_INTERMEDIATE_IMGS == 1
	fprintf(stderr, "SWT> Filtering 1: %d letters found\n", letters.nb_letters);
	dump_letters("swt_0010_letters_filter_1.ppm", img_in, &letters);
#endif

	possible_letters = letters;
	letters = filter_letters_by_center(&possible_letters);
#if OUTPUT_INTERMEDIATE_IMGS == 1
	fprintf(stderr, "SWT> Filtering 2: %d letters found\n", letters.nb_letters);
	dump_letters("swt_0011_letters_filter_2.ppm", img_in, &letters);
#endif
	free(possible_letters.letters);
	free(possible_letters.stats);

	chains = make_valid_pairs(&letters);
	x = merge_chains(&chains);
#if OUTPUT_INTERMEDIATE_IMGS == 1
	fprintf(stderr, "SWT> %d chains after merges\n", x);
#endif

	PRINT_TIME_FN();

	x = -1;
	switch(output_type) {
		case PF_SWT_OUTPUT_ORIGINAL_BOXES:
			x = render_chains_boxes(
					&swt_out.swt,
					img_in, img_out, &chains,
					output_type);
			break;
		case PF_SWT_OUTPUT_GRAYSCALE_TEXT:
		case PF_SWT_OUTPUT_BW_TEXT:
			x = render_chains_text(
					&swt_out.swt,
					img_in, img_out, &chains,
					output_type);
			break;
	}
	assert(x >= 0);
#if OUTPUT_INTERMEDIATE_IMGS == 1
	fprintf(stderr, "SWT> %d letters rendered\n", x);
#endif

	PRINT_TIME_FN();
	pf_dbl_matrix_free(&swt_out.swt);


	// Note: these structures share some allocations, so we have
	// to be careful about what we free
	for (x = 0 ; x < all_possible_letters.nb_letters ; x++)
		free(all_possible_letters.letters[x]);
	free(all_possible_letters.letters);
	free(all_possible_letters.stats);
	free(letters.letters);

	for (chain = chains.first, nchain = (chain ? chain->next : NULL) ;
			chain != NULL ;
			chain = nchain, nchain = (chain ? chain->next : NULL)) {
		for (link = chain->first, nlink = (link ? link->next : NULL) ;
				link != NULL ;
				link = nlink, nlink = (link ? link->next : NULL)) {
			free(link);
		}
		free(chain);
	}

	PRINT_TIME_FN();
}

#ifndef NO_PYTHON

PyObject *pyswt(PyObject *self, PyObject* args)
{
	int img_x, img_y;
	Py_buffer img_in, img_out;
	struct pf_bitmap bitmap_in;
	struct pf_bitmap bitmap_out;
	int output_type;

	if (!PyArg_ParseTuple(args, "iiy*y*i",
				&img_x, &img_y,
				&img_in,
				&img_out,
				&output_type)) {
		return NULL;
	}

	assert(img_x * img_y * 4 /* RGBA */ == img_in.len);
	assert(img_in.len == img_out.len);

	bitmap_in = from_py_buffer(&img_in, img_x, img_y);
	bitmap_out = from_py_buffer(&img_out, img_x, img_y);

	memset(bitmap_out.pixels, 0xFFFFFFFF, img_out.len);
	Py_BEGIN_ALLOW_THREADS;
	pf_swt(&bitmap_in, &bitmap_out, output_type);
	Py_END_ALLOW_THREADS;

	PyBuffer_Release(&img_in);
	PyBuffer_Release(&img_out);
	Py_RETURN_NONE;
}

#endif // !NO_PYTHON
