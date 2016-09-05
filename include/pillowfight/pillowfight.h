#ifndef __PILLOWFIGHT_H
#define __PILLOWFIGHT_H

#include <pillowfight/util.h>

#ifdef NO_PYTHON
extern void pf_ace(const struct pf_bitmap *in, struct pf_bitmap *out,
		int nb_samples, double slope, double limit,
		int nb_threads);

extern void pf_canny(const struct pf_bitmap *in, struct pf_bitmap *out);
#endif

extern struct pf_dbl_matrix pf_canny_on_matrix(const struct pf_dbl_matrix *in);

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

extern const struct pf_dbl_matrix g_pf_kernel_sobel_x;
extern const struct pf_dbl_matrix g_pf_kernel_sobel_y;
extern const struct pf_dbl_matrix g_pf_kernel_scharr_x;
extern const struct pf_dbl_matrix g_pf_kernel_scharr_y;

#define PF_SOBEL_DEFAULT_KERNEL_X (&g_pf_kernel_sobel_x)
#define PF_SOBEL_DEFAULT_KERNEL_Y (&g_pf_kernel_sobel_y)

struct pf_gradient_matrixes {
	struct pf_dbl_matrix g_x;
	struct pf_dbl_matrix g_y;
	struct pf_dbl_matrix intensity;
	struct pf_dbl_matrix direction;
};
extern struct pf_gradient_matrixes pf_sobel_on_matrix(const struct pf_dbl_matrix *in,
		const struct pf_dbl_matrix *kernel_x, const struct pf_dbl_matrix *kernel_y,
		double gaussian_sigma, int gaussian_stddev);

#ifdef NO_PYTHON
extern void pf_swt(const struct pf_bitmap *in_img, struct pf_bitmap *out_img);

extern void pf_unpaper_blackfilter(const struct pf_bitmap *in, struct pf_bitmap *out);

extern void pf_unpaper_blurfilter(const struct pf_bitmap *in, struct pf_bitmap *out);

extern void pf_unpaper_border(const struct pf_bitmap *in, struct pf_bitmap *out);

extern void pf_unpaper_masks(const struct pf_bitmap *in, struct pf_bitmap *out);

extern void pf_unpaper_noisefilter(const struct pf_bitmap *in, struct pf_bitmap *out);
#endif

extern void pf_write_bitmap_to_ppm(const char *filepath, const struct pf_bitmap *in);

#endif
