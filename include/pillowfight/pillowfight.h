#ifndef __PILLOWFIGHT_H
#define __PILLOWFIGHT_H

#include <stdint.h>

/*!<
 * - Everything here assume we work with RGBA images
 * - In position (x, y): x = width, y = height.
 */

enum pf_color {
	COLOR_R = 0,
	COLOR_G,
	COLOR_B,
	COLOR_A,
};
#define PF_NB_COLORS 4 /* to align on 32bits */
#define PF_NB_RGB_COLORS 3 /* when we want explicitly to ignore the alpha channel */

#define PF_WHITE 0xFF
#define PF_WHOLE_WHITE 0xFFFFFFFF
#define PF_BLACK 0x00


union pf_pixel {
	uint32_t whole;
	struct {
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	} color;
	uint8_t channels[4];
};


struct pf_bitmap {
	struct {
		int x;
		int y;
	} size;
	union pf_pixel *pixels;
};

/*!
 * \returns a uint32_t (RGBA)
 */
#define PF_GET_PIXEL(bitmap, a, b) ((bitmap)->pixels[((b) * (bitmap)->size.x) + (a)])
#define PF_GET_PIXEL_DEF(bitmap, a, b, def) \
	((a < 0 || a >= bitmap->size.x) ? def : \
	 ((b < 0 || b >= bitmap->size.y) ? def : \
	  PF_GET_PIXEL(bitmap, a, b)))

#define PF_SET_PIXEL(bitmap, a, b, value) PF_GET_PIXEL(bitmap, a, b).whole = (value);

/*!
 * \returns a uint8_t
 */
#define PF_GET_COLOR(bitmap, a, b, color) (PF_GET_PIXEL(bitmap, a, b).channels[(color)])
#define PF_GET_COLOR_DEF(bitmap, a, b, color, def) (PF_GET_PIXEL_DEF(bitmap, a, b, def).channels[(color)])

#define PF_SET_COLOR(bitmap, a, b, color, value) (PF_GET_PIXEL(bitmap, a, b).channels[(color)]) = (value)


/*!
 * \brief matrix of doubles
 */
struct pf_dbl_matrix {
	struct {
		int x;
		int y;
	} size;
	double *values;
};

#define PF_MATRIX_GET(matrix, a, b) ((matrix)->values[((b) * (matrix)->size.x) + (a)])
#define PF_MATRIX_SET(matrix, a, b, val) PF_MATRIX_GET(matrix, a, b) = (val);


#ifdef NO_PYTHON

#define PF_DEFAULT_ACE_SLOPE 10
#define PF_DEFAULT_ACE_LIMIT 1000
#define PF_DEFAULT_ACE_NB_SAMPLES 100
#define PF_DEFAULT_ACE_NB_THREADS 2
extern void pf_ace(const struct pf_bitmap *in, struct pf_bitmap *out,
		int nb_samples, double slope, double limit,
		int nb_threads);

extern void pf_canny(const struct pf_bitmap *in, struct pf_bitmap *out);
#endif

extern struct pf_dbl_matrix pf_canny_on_matrix(const struct pf_dbl_matrix *in);

#define PF_COMPARE_DEFAULT_TOLERANCE 10

#ifdef NO_PYTHON
extern int pf_compare(const struct pf_bitmap *in, const struct pf_bitmap *in2,
		struct pf_bitmap *out, int tolerance);
#endif

#define PF_GAUSSIAN_DEFAULT_SIGMA 2.0
#define PF_GAUSSIAN_DEFAULT_NB_STDDEV 5

#ifdef NO_PYTHON
extern void pf_gaussian(const struct pf_bitmap *in, struct pf_bitmap *out,
		double sigma, int nb_stddev);
#endif

extern struct pf_dbl_matrix pf_gaussian_on_matrix(
		const struct pf_dbl_matrix *grayscale_matrix,
		double sigma, int nb_stddev);

#ifdef NO_PYTHON
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

enum pf_swt_output
{
	PF_SWT_OUTPUT_BW_TEXT = 0,
	PF_SWT_OUTPUT_GRAYSCALE_TEXT,
	PF_SWT_OUTPUT_ORIGINAL_BOXES,
};
#define PF_DEFAULT_SWT_OUTPUT PF_SWT_OUTPUT_BW_TEXT

#ifdef NO_PYTHON
extern void pf_swt(const struct pf_bitmap *in_img, struct pf_bitmap *out_img,
		enum pf_swt_output output_type);

extern void pf_unpaper_blackfilter(const struct pf_bitmap *in, struct pf_bitmap *out);

extern void pf_unpaper_blurfilter(const struct pf_bitmap *in, struct pf_bitmap *out);

extern void pf_unpaper_border(const struct pf_bitmap *in, struct pf_bitmap *out);

extern void pf_unpaper_grayfilter(const struct pf_bitmap *in, struct pf_bitmap *out);

extern void pf_unpaper_masks(const struct pf_bitmap *in, struct pf_bitmap *out);

extern void pf_unpaper_noisefilter(const struct pf_bitmap *in, struct pf_bitmap *out);
#endif

extern void pf_write_bitmap_to_ppm(const char *filepath, const struct pf_bitmap *in);

#endif
