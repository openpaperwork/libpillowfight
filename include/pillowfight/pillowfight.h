#ifndef __PILLOWFIGHT_H
#define __PILLOWFIGHT_H

#include <pillowfight/util.h>

#ifdef NO_PYTHON
extern void pf_ace(const struct pf_bitmap *in, struct pf_bitmap *out,
		int nb_samples, double slope, double limit,
		int nb_threads);

extern void pf_canny(const struct pf_bitmap *in, struct pf_bitmap *out);
#endif

#define PF_GAUSSIAN_DEFAULT_SIGMA 2.0
#define PF_GAUSSIAN_DEFAULT_NB_STDDEV 5

#ifdef NO_PYTHON
extern void pf_gaussian(const struct pf_bitmap *in, struct pf_bitmap *out, double sigma, int nb_stddev);
#endif

extern struct pf_dbl_matrix pf_gaussian_on_matrix(
		const struct pf_dbl_matrix *grayscale_matrix, double sigma, int nb_stddev);

#ifdef NO_PYTHON
extern void pf_grayfilter(const struct pf_bitmap *in, struct pf_bitmap *out);

extern void pf_sobel(const struct pf_bitmap *in_img, struct pf_bitmap *out_img);
#endif

struct pf_gradient_matrixes {
	struct pf_dbl_matrix intensity;
	struct pf_dbl_matrix direction;
};
extern struct pf_gradient_matrixes pf_sobel_on_matrix(const struct pf_dbl_matrix *in);

#ifdef NO_PYTHON
extern void pf_unpaper_blackfilter(const struct pf_bitmap *in, struct pf_bitmap *out);

extern void pf_unpaper_blurfilter(const struct pf_bitmap *in, struct pf_bitmap *out);

extern void pf_unpaper_border(const struct pf_bitmap *in, struct pf_bitmap *out);

extern void pf_unpaper_masks(const struct pf_bitmap *in, struct pf_bitmap *out);

extern void pf_unpaper_noisefilter(const struct pf_bitmap *in, struct pf_bitmap *out);
#endif

#endif
