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
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <pillowfight/pillowfight.h>
#include <pillowfight/util.h>

#ifndef NO_PYTHON
#include "_pymod.h"
#endif

/*!
 * \brief Algorithm 'mask' from unpaper, partially rewritten.
 */

#define SCAN_SIZE 50
#define SCAN_STEP 5
#define SCAN_THRESHOLD 0.1
#define SCAN_MIN 100

#define MASK_COLOR PF_WHOLE_WHITE

/**
 * Returns the average brightness of a rectagular area.
 */
static int brightness_rect(const struct pf_bitmap *img, int x1, int y1, int x2, int y2)
{
    int x;
    int y;
    int total = 0;
    const int count = (x2 - x1) * (y2 - y1);

    for (x = x1; x < x2; x++) {
        for (y = y1; y < y2; y++) {
            total += PF_GET_PIXEL_GRAYSCALE(img, x, y);
        }
    }
    return total / count;
}

/**
 * Finds one edge of non-black pixels heading from one starting point towards edge direction.
 *
 * @return number of shift-steps until blank edge found
 */
static int detect_edge(const struct pf_bitmap *img, int start_x, int start_y, int shift_x)
{
	int left;
	int top;
	int right;
	int bottom;

	int total = 0;
	int count = 0;
	int scan_depth;

	double blackness, threshold;

	assert(shift_x != 0);

	// horizontal border is to be detected, vertical shifting of scan-bar
	// vertical border is to be detected, horizontal shifting of scan-bar
	scan_depth = img->size.y;
	left = start_x - (SCAN_SIZE / 2);
	top = start_y - (scan_depth / 2);
	right = start_x + (SCAN_SIZE / 2);
	bottom = start_y + (scan_depth / 2);

	while (1) {
		blackness = (double)(((int)PF_WHITE) - brightness_rect(img, left, top, right, bottom));
		total += blackness;
		count++;
		// is blackness below threshold*average?
		threshold = ((((double)SCAN_THRESHOLD) * total) / count);
		if ((blackness < threshold)
				// this will surely become true when pos reaches
				// the outside of the actual image area and
				// blacknessRect() will deliver 0 because all
				// pixels outside are considered white
				|| (((int)blackness) == 0)) {
			return count; // return here, return absolute value of shifting difference
		}
		left += shift_x;
		right += shift_x;
	}
}

static struct pf_rectangle detect_mask(const struct pf_bitmap *img, int x, int y)
{
	int width;
	struct pf_rectangle out;
	int edge;

	memset(&out, 0, sizeof(out));
	out.a.x = -1;
	out.a.y = -1;
	out.b.x = -1;
	out.b.y = -1;

	/* we work horizontally only */
	edge = detect_edge(img, x, y, -SCAN_STEP);
	out.a.x = (x
			- (SCAN_STEP * edge)
			- (SCAN_SIZE / 2));
	edge = detect_edge(img, x, y, SCAN_STEP);
	out.b.x = (x
			+ (SCAN_STEP * edge)
			+ (SCAN_SIZE / 2));

	// we don't work vertically --> full range of sheet
	out.a.y = 0;
	out.b.y = img->size.y;

	// if below minimum or above maximum, set to maximum
	width = out.b.x - out.a.x;
	if (width < SCAN_MIN || width >= img->size.x) {
		out.a.x = 0;
		out.b.x = img->size.x;
	}
	return out;
}

#ifndef NO_PYTHON
static
#endif
void pf_unpaper_masks(const struct pf_bitmap *in, struct pf_bitmap *out)
{
	struct pf_rectangle mask;

	memcpy(out->pixels, in->pixels, sizeof(union pf_pixel) * in->size.x * in->size.y);

	mask = detect_mask(in, in->size.x / 2, in->size.y /2);
	pf_apply_mask(out, &mask);
}

#ifndef NO_PYTHON

PyObject *pymasks(PyObject *self, PyObject* args)
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
	Py_BEGIN_ALLOW_THREADS;
	pf_unpaper_masks(&bitmap_in, &bitmap_out);
	Py_END_ALLOW_THREADS;

	PyBuffer_Release(&img_in);
	PyBuffer_Release(&img_out);
	Py_RETURN_NONE;
}

#endif // !NO_PYTHON
