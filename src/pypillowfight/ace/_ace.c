#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <values.h>

#include <Python.h>

#include <util.h>

#define ACE_COLORS 3 /* we don't use the alpha channel */

#define MATRIX_GET(matrix, a, b, color) \
	((matrix)->values[((b) * matrix->size.x) + (a)].channels[color])
#define MATRIX_SET(matrix, a, b, color, val) \
	((matrix)->values[((b) * matrix->size.x) + (a)].channels[color]) = (val)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))


/*!
 * 2d matrix, with 3 channels for each element of the matrix
 * (so .. basically a 3d matrix)
 */
struct matrix {
	struct {
		int x;
		int y;
	} size;
	union {
		uint32_t whole;
		struct {
			uint8_t r;
			uint8_t g;
			uint8_t b;
			uint8_t a;
		};
		uint8_t channels[NB_COLORS];
	} values[];
};

struct rscore {
	double max[NB_COLORS];
	double min[NB_COLORS];
	struct matrix *scores;
};

struct pair {
	int x;
	int y;
};

static struct matrix *new_matrix(int x, int y) {
	struct matrix *out = calloc(1,
			sizeof(struct matrix) + (x * y * NB_COLORS * sizeof(double))
	);
	if (out == NULL)
		abort();
	out->size.x = x;
	out->size.y = y;
	return out;
}

static void free_matrix(struct matrix *matrix) {
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
		double _diff_slope = diff * slope; \
		(out) = (_diff_slope > (limit)) ? (limit) : ( \
			(_diff_slope < (-limit)) ? (-limit) : _diff_slope \
		); \
	} while(0)

#define GET_SLOPE(in_max, in_min, out_max, out_min) \
	(((out_max) - (out_min)) / ((in_max) - (in_min)))

#define GET_LINEAR_SCALING(v, in_max, in_min, out_max, out_min) \
	((((v) - (in_min)) * GET_SLOPE(in_max, in_min, out_max, out_min)) + (out_min))

static void _ace(const struct bitmap *in, struct bitmap *out,
		int nb_samples, double slope, double limit)
{
	int i, j, sample_idx;
	int color;
	struct pair sample;
	double rscore_sums[ACE_COLORS];
	struct rscore rscore;
	double dist;
	double saturation;
	double denominator;
	double scaled;

	struct pair *samples;

	samples = create_pairs(nb_samples, in->size.x, in->size.y);
	rscore = new_rscore(in->size.x, in->size.y);

	// Chromatic/Spatial adjustment
	for (i = 0 ; i < in->size.x ; i++) {
		for (j = 0 ; j < in->size.y ; j++) {
			memset(rscore_sums, 0, sizeof(rscore_sums));

			denominator = 0.0;

			for (sample_idx = 0 ; sample_idx < nb_samples ; sample_idx++) {
				sample = samples[sample_idx];
				dist = sqrt(
						((i - sample.x) * (i - sample.x))
						+ ((j - sample.y) * (j - sample.y))
					);
				if (dist < (in->size.y / 5))
					continue;
				for (color = 0 ; color < ACE_COLORS ; color++) {
					GET_SATURATION(saturation,
							GET_COLOR(in, i, j, color)
							- GET_COLOR(in, sample.x, sample.y, color),
							slope,
							limit);
					rscore_sums[color] += saturation;
				}
				denominator += (limit / dist);
			}

			for (color = 0 ; color < ACE_COLORS ; color++) {
				rscore_sums[color] /= denominator;
				MATRIX_SET(rscore.scores, i, j, color, rscore_sums[color]);
				rscore.max[color] = MAX(rscore.max[color], rscore_sums[color]);
				rscore.min[color] = MIN(rscore.max[color], rscore_sums[color]);
			}
		}
	}

	free_pairs(samples);

	// Dynamic tone reproduction scaling
	for (i = 0 ; i < in->size.x ; i++) {
		for (j = 0 ; j < in->size.y ; j++) {
			for (color = 0 ; color < ACE_COLORS ; color++) {
				scaled = GET_LINEAR_SCALING(
					MATRIX_GET(rscore.scores, i, j, color),
					rscore.max[color], rscore.min[color],
					255.0, 0.0
				);
				SET_COLOR(out, i, j, color, (uint16_t)scaled);
			}
		}
	}

	free_rscore(&rscore);
}

static PyObject *ace(PyObject *self, PyObject* args)
{
	int img_x, img_y;
	Py_buffer img_in, img_out;
	double slope, limit;
	int samples, seed;
	struct bitmap bitmap_in;
	struct bitmap bitmap_out;

	if (!PyArg_ParseTuple(args, "iiy*ddiiy*",
				&img_x, &img_y,
				&img_in, &slope, &limit,
				&samples, &seed, &img_out)) {
		return NULL;
	}

	assert(img_x * img_y * 4 /* RGBA */ == img_in.len);
	assert(img_in.len == img_out.len);

	srand(seed);

	bitmap_in = from_py_buffer(&img_in, img_x, img_y);
	bitmap_out = from_py_buffer(&img_out, img_x, img_y);

	_ace(&bitmap_in, &bitmap_out, samples, slope, limit);

	PyBuffer_Release(&img_in);
	PyBuffer_Release(&img_out);
	Py_RETURN_NONE;
}


static PyMethodDef ace_methods[] = {
	{"ace", ace, METH_VARARGS, NULL},
	{NULL, NULL, 0, NULL},
};

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
