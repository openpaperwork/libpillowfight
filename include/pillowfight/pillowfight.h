#ifndef __PILLOWFIGHT_H
#define __PILLOWFIGHT_H

#include <pillowfight/util.h>

#ifdef NO_PYTHON
extern void ace(const struct bitmap *in, struct bitmap *out,
		int nb_samples, double slope, double limit,
		int nb_threads);

extern void blackfilter(const struct bitmap *in, struct bitmap *out);

extern void blurfilter(const struct bitmap *in, struct bitmap *out);

extern void border(const struct bitmap *in, struct bitmap *out);

extern void canny(const struct bitmap *in, struct bitmap *out);
#endif

#define PF_GAUSSIAN_DEFAULT_SIGMA 2.0
extern void gaussian(const struct bitmap *in, struct bitmap *out, double sigma, int nb_stddev);

#ifdef NO_PYTHON
extern void grayfilter(const struct bitmap *in, struct bitmap *out);

extern void masks(const struct bitmap *in, struct bitmap *out);

extern void noisefilter(const struct bitmap *in, struct bitmap *out);

extern void sobel(const struct bitmap *in_img, struct bitmap *out_img);
#endif

#endif
