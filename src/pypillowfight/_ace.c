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
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <values.h>

#include <Python.h>

#include "util.h"

/*!
 * \brief Automatic color equalization algorithm
 *
 * A. Rizzi, C. Gatta and D. Marini
 * - "A new algorithm for unsupervised global and local color correction."
 *
 * This is a parallelized version of the algorithm, using all the cores/processors
 * of the computer (up to 32).
 */

#define MAX_THREADS 32

#define ACE_COLORS 3 /* we don't use the alpha channel */

#define DBL_MATRIX_GET(matrix, a, b, color) \
	((matrix)->values[((b) * (matrix)->size.x) + (a)].channels[color])

#define DBL_MATRIX_SET(matrix, a, b, color, val) \
	DBL_MATRIX_GET(matrix, a, b, color) = (val)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/*!
 * 2d matrix, with 3 channels for each element of the matrix
 * (so .. basically a 3d matrix)
 */
struct dbl_matrix {
	struct {
		int x;
		int y;
	} size;
	union {
		struct {
			double r;
			double g;
			double b;
			double a;
		};
		double channels[NB_COLORS];
	} values[];
};

struct rscore {
	double max[NB_COLORS];
	double min[NB_COLORS];
	struct dbl_matrix *scores;
};

struct pair {
	int x;
	int y;
};

static struct dbl_matrix *new_matrix(int x, int y) {
	struct dbl_matrix *out = calloc(1,
			sizeof(struct dbl_matrix) + (x * y * NB_COLORS * sizeof(double))
	);
	if (out == NULL)
		abort();
	out->size.x = x;
	out->size.y = y;
	return out;
}

static void free_matrix(struct dbl_matrix *matrix) {
	free(matrix);
}

static struct rscore new_rscore(int x, int y) {
	struct rscore out;

	int i;

	memset(&out, 0, sizeof(out));
	for (i = 0 ; i < NB_COLORS ; i++)
		out.max[i] = 0.0;
	for (i = 0 ; i < NB_COLORS ; i++)
		out.min[i] = MAXDOUBLE;
	out.scores = new_matrix(x, y);

	return out;
}

static void free_rscore(struct rscore *rscore) {
	free_matrix(rscore->scores);
}

static struct pair *create_pairs(int nb_pairs, int max_x, int max_y) {
	struct pair *out;
	int i;

	out = malloc(nb_pairs * sizeof(struct pair));
	if (out == NULL)
		abort();
	for (i = 0 ; i < nb_pairs ; i++) {
		out[i].x = rand() % max_x; // TODO(JFlesch): could be better
		out[i].y = rand() % max_y; // TODO(JFlesch): could be better
	}

	return out;
}

static void free_pairs(struct pair *pairs) {
	free(pairs);
}

#define GET_SATURATION(out, diff, slope, limit) \
	do { \
		(out) = (diff) * (slope); \
		(out) = ((out) > (limit)) ? (limit) : ( \
			((out) < (-limit)) ? (-limit) : (out) \
		); \
	} while(0)

#define GET_SLOPE(in_max, in_min, out_max, out_min) \
	(((out_max) - (out_min)) / ((in_max) - (in_min)))

#define GET_LINEAR_SCALING(v, in_max, in_min, out_max, out_min) \
	((((v) - (in_min)) * GET_SLOPE(in_max, in_min, out_max, out_min)) + (out_min))

struct ace_thread_adjustment_params {
	struct {
		int x;
		int y;
	} start;
	struct {
		int x;
		int y;
	} stop;

	double slope;
	double limit;
	const struct bitmap *in;
	const struct pair *samples;
	int nb_samples;

	/* output */
	struct rscore rscore;
};

static void *ace_thread_adjustment(void *_thread_params)
{
	int i, j, color, sample_idx;
	struct ace_thread_adjustment_params *params = _thread_params;
	struct pair sample;
	double dist;
	double denominator;
	double rscore_sums[ACE_COLORS];
	double saturation;

	for (i = params->start.x ; i < params->stop.x ; i++) {
		for (j = params->start.y ; j < params->stop.y ; j++) {
			memset(rscore_sums, 0, sizeof(rscore_sums));
			denominator = 0.0;

			for (sample_idx = 0 ; sample_idx < params->nb_samples ; sample_idx++) {
				sample = params->samples[sample_idx];

				dist = sqrt(
						((i - sample.x) * (i - sample.x))
						+ ((j - sample.y) * (j - sample.y))
					);
				if (dist < (params->in->size.y / 5))
					continue;
				for (color = 0 ; color < ACE_COLORS ; color++) {
					GET_SATURATION(saturation,
							GET_COLOR(params->in, i, j, color)
							- GET_COLOR(params->in, sample.x, sample.y, color),
							params->slope,
							params->limit);
					saturation /= dist;
					rscore_sums[color] += saturation;
				}
				denominator += (params->limit / dist);
			}

			for (color = 0 ; color < ACE_COLORS ; color++) {
				rscore_sums[color] /= denominator;
				DBL_MATRIX_SET(params->rscore.scores, i, j, color, rscore_sums[color]);

				params->rscore.max[color] = MAX(
						params->rscore.max[color], rscore_sums[color]
				);
				params->rscore.min[color] = MIN(
						params->rscore.min[color], rscore_sums[color]
				);
			}
		}
	}

	return params;
}

struct ace_thread_scaling_params {
	struct {
		int x;
		int y;
	} start;
	struct {
		int x;
		int y;
	} stop;

	const struct rscore *rscore;

	const struct bitmap *out;
};

void *ace_thread_scaling(void *_params) {
	struct ace_thread_scaling_params *params = _params;
	int i, j, color;
	double scaled;

	for (i = params->start.x ; i < params->stop.x ; i++) {
		for (j = params->start.y ; j < params->stop.y ; j++) {
			for (color = 0 ; color < ACE_COLORS ; color++) {
				scaled = DBL_MATRIX_GET(params->rscore->scores, i, j, color);
				scaled = GET_LINEAR_SCALING(
					scaled,
					params->rscore->max[color],
					params->rscore->min[color],
					255.0, 0.0
				);
				SET_COLOR(params->out, i, j, color, (uint8_t)scaled);
			}
		}
	}

	return params;
}

static void ace_main(const struct bitmap *in, struct bitmap *out,
		int nb_samples, double slope, double limit,
		int nb_threads)
{
	int i, nb_lines_per_thread, color;
	struct rscore rscore;
	struct pair *samples;
	pthread_t threads[MAX_THREADS];
	struct ace_thread_adjustment_params *adj_params;
	struct ace_thread_scaling_params *scaling_params;

	if (nb_threads > MAX_THREADS)
		nb_threads = MAX_THREADS;
	if (nb_threads > in->size.y)
		nb_threads = 1;

	samples = create_pairs(nb_samples, in->size.x, in->size.y);
	rscore = new_rscore(in->size.x, in->size.y);

	memset(&threads, 0, sizeof(threads));

	nb_lines_per_thread = in->size.y / nb_threads;

	// Chromatic/Spatial adjustment
	for (i = 0 ; i < nb_threads ; i++) {
		adj_params = calloc(1, sizeof(struct ace_thread_adjustment_params));

		adj_params->start.x = 0;
		adj_params->start.y = (i * nb_lines_per_thread);
		adj_params->stop.x = in->size.x;
		adj_params->stop.y = MIN((i + 1) * nb_lines_per_thread, in->size.y);

		adj_params->slope = slope;
		adj_params->limit = limit;
		adj_params->in = in;
		adj_params->samples = samples;
		adj_params->nb_samples = nb_samples;

		// Will share the pointer to the same matrix
		// but not the same min/max values
		memcpy(&adj_params->rscore, &rscore, sizeof(rscore));

		pthread_create(&threads[i], NULL, ace_thread_adjustment, adj_params);
	}

	for (i = 0 ; i < nb_threads ; i++) {
		pthread_join(threads[i], (void **)&adj_params);
		// we must merge the min/max values
		for (color = 0 ; color < ACE_COLORS ; color++)
			rscore.max[color] = MAX(rscore.max[color], adj_params->rscore.max[color]);
		for (color = 0 ; color < ACE_COLORS ; color++)
			rscore.min[color] = MIN(rscore.min[color], adj_params->rscore.min[color]);
	}

	free_pairs(samples);

	// Dynamic tone reproduction scaling
	for (i = 0 ; i < nb_threads ; i++) {
		scaling_params = calloc(1, sizeof(struct ace_thread_scaling_params));

		scaling_params->start.x = 0;
		scaling_params->start.y = (i * nb_lines_per_thread);
		scaling_params->stop.x = in->size.x;
		scaling_params->stop.y = MIN((i + 1) * nb_lines_per_thread, in->size.y);
		scaling_params->rscore = &rscore;

		scaling_params->out = out;

		pthread_create(&threads[i], NULL, ace_thread_scaling, scaling_params);
	}

	for (i = 0 ; i < nb_threads ; i++) {
		pthread_join(threads[i], NULL);
	}

	free_rscore(&rscore);
}

static PyObject *ace(PyObject *self, PyObject* args)
{
	int img_x, img_y;
	Py_buffer img_in, img_out;
	double slope, limit;
	int samples, seed;
	int nb_threads;
	struct bitmap bitmap_in;
	struct bitmap bitmap_out;

	if (!PyArg_ParseTuple(args, "iiy*ddiiiy*",
				&img_x, &img_y,
				&img_in, &slope, &limit,
				&samples, &nb_threads,
				&seed, &img_out)) {
		return NULL;
	}

	assert(img_x * img_y * 4 /* RGBA */ == img_in.len);
	assert(img_in.len == img_out.len);

	srand(seed);

	bitmap_in = from_py_buffer(&img_in, img_x, img_y);
	bitmap_out = from_py_buffer(&img_out, img_x, img_y);

	ace_main(&bitmap_in, &bitmap_out, samples, slope, limit, nb_threads);

	PyBuffer_Release(&img_in);
	PyBuffer_Release(&img_out);
	Py_RETURN_NONE;
}


static PyMethodDef ace_methods[] = {
	{"ace", ace, METH_VARARGS, NULL},
	{NULL, NULL, 0, NULL},
};

#if PY_VERSION_HEX < 0x03000000

PyMODINIT_FUNC
init_ace(void)
{
    PyObject* m = Py_InitModule("_ace", ace_methods);
}

#else

static struct PyModuleDef ace_module = {
	PyModuleDef_HEAD_INIT,
	"_ace",
	NULL /* doc */,
	-1,
	ace_methods,
};

PyMODINIT_FUNC PyInit__ace(void)
{
	return PyModule_Create(&ace_module);
}

#endif
