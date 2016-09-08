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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pillowfight/pillowfight.h>
#include <pillowfight/util.h>

#ifndef NO_PYTHON
#include "_pymod.h"
#endif

#define SCAN_SIZE 100
#define SCAN_STEP 50
#define INTENSITY 0.01
#define PF_WHITE_THRESHOLD 0.9
#define PF_WHITE_MIN ((int)(PF_WHITE_THRESHOLD * PF_WHITE))

/*!
 * \brief Algorithm blurfilter from unpaper, partially rewritten.
 */

#ifndef NO_PYTHON
static
#endif
void pf_unpaper_blurfilter(const struct pf_bitmap *in, struct pf_bitmap *out)
{
	int left;
	int top;
	int right;
	int bottom;
	int count;
	int max_left;
	int max_top;
	int blocks_per_row;
	int *prev_counts; // Number of dark pixels in previous row
	int *cur_counts; // Number of dark pixels in current row
	int *next_counts; // Number of dark pixels in next row
	int *tmp_counts; // used for permutations of pointers
	int block; // Number of block in row; Counting begins with 1
	int max;
	int total; // Number of pixels in a block
	int result;

	memcpy(out->pixels, in->pixels, sizeof(union pf_pixel) * in->size.x * in->size.y);

	result = 0;
	left = 0;
	top = 0;
	right = SCAN_SIZE - 1;
	bottom = SCAN_SIZE - 1;
	max_left = out->size.x - SCAN_SIZE;
	max_top = out->size.y - SCAN_SIZE;

	blocks_per_row = out->size.x / SCAN_SIZE;
	// allocate one extra block left and right
	prev_counts = calloc(blocks_per_row + 2, sizeof(int));
	cur_counts = calloc(blocks_per_row + 2, sizeof(int));
	next_counts = calloc(blocks_per_row + 2, sizeof(int));

	total = SCAN_SIZE * SCAN_SIZE;

	block = 1;
	for (left = 0; left <= max_left; left += SCAN_SIZE) {
		cur_counts[block] = pf_count_pixels_rect(left, top, right, bottom, PF_WHITE_MIN, out);
		block++;
		right += SCAN_SIZE;
	}
	cur_counts[0] = total;
	cur_counts[blocks_per_row] = total;
	next_counts[0] = total;
	next_counts[blocks_per_row] = total;

	// Loop through all blocks. For a block calculate the number of dark
	// pixels in this block, the number of dark pixels in the block in the
	// top-left corner and similarly for the block in the top-right,
	// bottom-left and bottom-right corner. Take the maximum of these
	// values. Clear the block if this number is not large enough compared
	// to the total number of pixels in a block.
	for (top = 0 ; top <= max_top ; top += SCAN_SIZE) {
		left = 0;
		right = SCAN_SIZE - 1;
		next_counts[0] = pf_count_pixels_rect(
				left, top + SCAN_STEP, right, bottom + SCAN_SIZE, PF_WHITE_MIN, out
		);

		block = 1;
		for (left = 0; left <= max_left; left += SCAN_SIZE) {
			// current block
			count = cur_counts[block];
			max = count;
			// top left
			count = prev_counts[block-1];
			if (count > max) {
				max = count;
			}
			// top right
			count = prev_counts[block+1];
			if (count > max) {
				max = count;
			}
			// bottom left
			count = next_counts[block-1];
			if (count > max) {
				max = count;
			}
			// bottom right (has still to be calculated)
			next_counts[block + 1] = pf_count_pixels_rect(
					left + SCAN_SIZE,
					top + SCAN_STEP,
					right + SCAN_SIZE,
					bottom + SCAN_SIZE,
					PF_WHITE_MIN,
					out
			);
			if (count > max) {
				max = count;
			}
			if ((((float)max) / total) <= INTENSITY) { // Not enough dark pixels
				pf_clear_rect(out, left, top, right, bottom);
				result += cur_counts[block];
				cur_counts[block] = total; // Update information
			}

			right += SCAN_SIZE;
			block++;
		}

		bottom += SCAN_SIZE;
		// Switch Buffers
		tmp_counts = prev_counts;
		prev_counts = cur_counts;
		cur_counts = next_counts;
		next_counts = tmp_counts;
	}
	free(prev_counts);
	free(cur_counts);
	free(next_counts);
}

#ifndef NO_PYTHON
PyObject *pyblurfilter(PyObject *self, PyObject* args)
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

	Py_BEGIN_ALLOW_THREADS;
	pf_unpaper_blurfilter(&bitmap_in, &bitmap_out);
	Py_END_ALLOW_THREADS;

	PyBuffer_Release(&img_in);
	PyBuffer_Release(&img_out);
	Py_RETURN_NONE;
}

#endif // !NO_PYTHON
