#ifndef __PILLOWFIGHT_H
#define __PILLOWFIGHT_H

#include <pillowfight/util.h>

#ifdef NO_PYTHON
extern void pf_ace(const struct bitmap *in, struct bitmap *out,
		int nb_samples, double slope, double limit,
		int nb_threads);

extern void pf_canny(const struct bitmap *in, struct bitmap *out);
#endif

#define PF_GAUSSIAN_DEFAULT_SIGMA 2.0
#define PF_GAUSSIAN_DEFAULT_NB_STDDEV 5
extern void pf_gaussian(const struct bitmap *in, struct bitmap *out, double sigma, int nb_stddev);

#ifdef NO_PYTHON
extern void pf_grayfilter(const struct bitmap *in, struct bitmap *out);

extern void pf_sobel(const struct bitmap *in_img, struct bitmap *out_img);

extern void pf_unpaper_blackfilter(const struct bitmap *in, struct bitmap *out);

extern void pf_unpaper_blurfilter(const struct bitmap *in, struct bitmap *out);

extern void pf_unpaper_border(const struct bitmap *in, struct bitmap *out);

extern void pf_unpaper_masks(const struct bitmap *in, struct bitmap *out);

extern void pf_unpaper_noisefilter(const struct bitmap *in, struct bitmap *out);
#endif

#endif
