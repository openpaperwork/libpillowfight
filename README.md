# pillowfight

Library containing various image processing algorithms.
It includes Python 3 bindings designed to operate on Pillow images (PIL.Image).

The C library depends only on the libc.
The Python bindings depend only on Pillow.

APIs are designed to be as simple to use as possible. Default values are provided
for every parameters.

Python 2.x is *not* supported.

Available algorithms are listed below.

## Available algorithms

* Unpaper's algorithms
  * Blackfilter
  * Noisefilter
  * Blurfilter
  * Masks
  * Grayfilter
  * Border
* ACE (Automatic Color Equalization ; Parallelized implementation)
* SWT (Stroke Width Transformation)


## Python API

The Python API can be compiled, installed and used without installing the C library.

### Installation

Latest release :

```sh
$ sudo pip3 install pypillowfight
```

Development version :

```sh
$ git clone https://github.com/jflesch/libpillowfight.git
$ cd libpillowfight
$ sudo python3 ./setup.py install
```

### Usage

For each algorithm, a function is available. It takes a PIL.Image instance as parameter.
It may take other optionnal parameters. The return value is another PIL.Image instance.

Example:

```py
import pillowfight

input_img = PIL.Image.open("tests/data/brightness_problem.jpg")
output_img = pillowfight.ace(input_img)
```

### Tests

```sh
$ sudo python3 ./setup.py nosetests
```


## C library

### Installation

```sh
$ mkdir cbuild
$ cd cbuild
$ cmake ..
$ make -j4
$ sudo make install
```

### Usage

#### C code

For each algorithm, a function is available. It takes a ```struct pf_bitmap```
as input. As output, it fills in another ```struct pf_bitmap```.

```struct pf_bitmap``` is a really simple structure:

```C
struct pf_bitmap {
	struct {
		int x;
		int y;
	} size;
	union pf_pixel *pixels;
};
```

```(struct pf_bitmap).size.x``` is the width of the image.

```(struct pf_bitmap).size.y``` is the height of the image.

```union pf_pixel``` are basically 32 bits integers, defined in a manner convenient
to retrieve each color independantly (RGB). Each color is on one byte. 4 byte is
unused (no alpha channel taken into account).

```(struct pf_bitmap).pixels``` must points to a memory area containing the image.
The image must contains ```x * y * union pf_pixel```.


#### Compilation with GCC

```
$ gcc -Wall -Werror -lpillowfight -o test test.c
```


## Note regarding Unpaper's algorithms

Many algorithms in this library are re-implementations of algorithms used
by [Unpaper](https://github.com/Flameeyes/unpaper). To make the API simpler
to use (.. and implement), a lot of settings have been hard-coded.

Unpaper applies them in the following order:

* Blackfilter
* Noisefilter
* Blurfilter
* Masks
* Grayfilter
* Border

I would advise applying automatic color equalization (ACE) first.

A basic documentation for some of the algorithms can be found in
[Unpaper's documentation](https://github.com/Flameeyes/unpaper/blob/master/doc/basic-concepts.md).

| Input | Output |
| ----- | ------ |
| [Black border problem](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem.jpg) | [ACE + Unpapered](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem_all.jpg) |
| [Brightness problem](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/brightness_problem.jpg) | [ACE + Unpapered](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/brightness_problem_all.jpg) |

## Available algorithms

### Automatic Color Equalization (ACE)

| Input | Output |
| ----- | ------ |
| [Brightness problem](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/brightness_problem.jpg) | [Corrected](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/brightness_problem_ace.jpg) |

This algorithm is quite slow (~40s for one big image with one thread
on my machine). So this version is parallelized (down to ~15s on a 4
cores computer).


#### Python API

```py
out_img = pillowfight.ace(img_in,
	slope=10,
	limit=1000,
	samples=100,
	seed=None)
```

Use as many threads as there are cores on the computer (up to 32).

This algorithm uses random number. If you need consistent results
(for unit tests for instance), you can specify a seed for the
random number generator. Otherwise, time.time() will be used.


#### C API

```C
#define PF_DEFAULT_ACE_SLOPE 10
#define PF_DEFAULT_ACE_LIMIT 1000
#define PF_DEFAULT_ACE_NB_SAMPLES 100
#define PF_DEFAULT_ACE_NB_THREADS 2
extern void pf_ace(const struct pf_bitmap *in, struct pf_bitmap *out,
		int nb_samples, double slope, double limit,
		int nb_threads);
```

This function uses random numbers coming (```rand()```).
You *should* call ```srand()``` before calling this function.


#### Sources

* "A new algorithm for unsupervised global and local color correction." - A. Rizzi, C. Gatta and D. Marini
* http://argmax.jp/index.php?colorcorrect


### Canny's edge detection

| Input | Output |
| ----- | ------ |
| [Crappy background](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/crappy_background.jpg) | [Canny output](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/crappy_background_canny.jpg) |


#### Python API

```py
img_out = pillowfight.canny(img_in)
```


#### C API

```C
extern void pf_canny(const struct pf_bitmap *in, struct pf_bitmap *out);
```


#### Sources

* "A computational Approach to Edge Detection" - John Canny
* https://en.wikipedia.org/wiki/Canny_edge_detector


### Compare

Simple algorithm showing the difference between two images.
Note that it converts the images to grayscale first.

It accepts a parameter 'tolerance': For each pixel, the difference with
the corresponding pixel is computed. If it difference is between 0 and
'tolerance', it is ignored (pixels are considered equal).

| Input | Input2 | Output |
| ----- | ------ | ------ |
| [Black border problem](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem.jpg) | [Black border problem + blackfilter](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem_blackfilter.jpg) | [Diff](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem_diff.jpg) |

#### Python API

```py
(nb_diff, out_img) = pillowfight.compare(img_in, img_in2, tolerance=10)
```


#### C API

```C
extern int pf_compare(const struct pf_bitmap *in, const struct pf_bitmap *in2,
		struct pf_bitmap *out, int tolerance);
```

Returns the number of pixels that are different between both images.


#### Sources

* "A new algorithm for unsupervised global and local color correction." - A. Rizzi, C. Gatta and D. Marini
* http://argmax.jp/index.php?colorcorrect


### Gaussian

| Input | Output |
| ----- | ------ |
| [Crappy background](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/crappy_background.jpg) | [Gaussed](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/crappy_background_gaussian.jpg) |

One of the parameters is ```sigma```. If it is equals to 0.0, it will be computed automatically
using the following formula (same as OpenCV):

```C
sigma = 0.3 * ((nb_stddev - 1) * 0.5 - 1) + 0.8;
```

#### Python API

```py
img_out = pillowfight.gaussian(img_in, sigma=2.0, nb_stddev=5)
```


#### C API

```
extern void pf_gaussian(const struct pf_bitmap *in, struct pf_bitmap *out,
	double sigma, int nb_stddev);
```


#### Sources

* https://en.wikipedia.org/wiki/Gaussian_blur
* https://en.wikipedia.org/wiki/Gaussian_function


### Sobel operator

| Input | Output |
| ----- | ------ |
| [Crappy background](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/crappy_background.jpg) | [Sobel](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/crappy_background_sobel.jpg) |


#### Python API

```py
img_out = pillowfight.sobel(img_in)
```


#### C API

```C
extern void pf_sobel(const struct pf_bitmap *in_img, struct pf_bitmap *out_img);
```


#### Sources

* https://www.researchgate.net/publication/239398674_An_Isotropic_3_3_Image_Gradient_Operator
* https://en.wikipedia.org/wiki/Sobel_operator


### Stroke Width Transformation

This algorithm extracts text from natural scenes images.

To find text, it looks for strokes. Note that it doesn't appear to work well on
scanned documents because strokes are too small.

This implementation can provide the output in 3 different ways:

* Black & White : Detected text is black. Background is white.
* Grayscale : Detected text is gray. Its exact color is proportional to the stroke width detected.
* Original boxes : The rectangle around the detected is copied as is in the output image. Rest of the image is white.

(following examples are with original boxes)

| Input | Output |
| ----- | ------ |
| [Black border problen](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem.jpg) | [SWT (SWT_OUTPUT_ORIGINAL_BOXES)](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem_swt.jpg) |
| [Crappy background](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/crappy_background.jpg) | [SWT (SWT_OUTPUT_ORIGINAL_BOXES)](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/crappy_background_swt.jpg) |
| [Black border problen](https://raw.githubusercontent.com/jflesch/libpillowfight/47b1f59ce9a5fb3816e3abd186c28cc4c6092e13/tests/data/black_border_problem.jpg) | [SWT (SWT_OUTPUT_BW_TEXT)](https://raw.githubusercontent.com/jflesch/libpillowfight/47b1f59ce9a5fb3816e3abd186c28cc4c6092e13/tests/data/black_border_problem_swt.jpg) |
| [Crappy background](https://raw.githubusercontent.com/jflesch/libpillowfight/47b1f59ce9a5fb3816e3abd186c28cc4c6092e13/tests/data/crappy_background.jpg) | [SWT (SWT_OUTPUT_BW_TEXT)](https://raw.githubusercontent.com/jflesch/libpillowfight/47b1f59ce9a5fb3816e3abd186c28cc4c6092e13/tests/data/crappy_background_swt.jpg) |


#### Python API

```py
# SWT_OUTPUT_BW_TEXT = 0  # default
# SWT_OUTPUT_GRAYSCALE_TEXT = 1
# SWT_OUTPUT_ORIGINAL_BOXES = 2

img_out = pillowfight.swt(img_in, output_type=pillowfight.SWT_OUTPUT_ORIGINAL_BOXES)
```


#### C API

```C
enum pf_swt_output
{
	PF_SWT_OUTPUT_BW_TEXT = 0,
	PF_SWT_OUTPUT_GRAYSCALE_TEXT,
	PF_SWT_OUTPUT_ORIGINAL_BOXES,
};
#define PF_DEFAULT_SWT_OUTPUT PF_SWT_OUTPUT_BW_TEXT

extern void pf_swt(const struct pf_bitmap *in_img, struct pf_bitmap *out_img,
		enum pf_swt_output output_type);
```


#### Sources

* ["Detecting Text in Natural Scenes with Stroke Width Transform"](http://cmp.felk.cvut.cz/~cernyad2/TextCaptchaPdf/Detecting%20Text%20in%20Natural%20Scenes%20with%20Stroke%20Width%20Transform.pdf) - Boris Epshtein, Eyal Ofek, Yonatan Wexler
* https://github.com/aperrau/DetectText


### Unpaper's Blackfilter

| Input | Output | Diff |
| ----- | ------ | ---- |
| [Black border problem](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem.jpg) | [Filtered](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem_blackfilter.jpg) | [Diff](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem_blackfilter_diff.jpg) |


#### Python API

```py
img_out = pillowfight.unpaper_blackfilter(img_in)
```


#### C API

```C
extern void pf_unpaper_blackfilter(const struct pf_bitmap *in, struct pf_bitmap *out);
```


#### Sources

* https://github.com/Flameeyes/unpaper
* https://github.com/Flameeyes/unpaper/blob/master/doc/basic-concepts.md


### Unpaper's Blurfilter

| Input | Output | Diff |
| ----- | ------ | ---- |
| [Black border problem](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem.jpg) | [Filtered](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem_blurfilter.jpg) | [Diff](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem_blurfilter_diff.jpg) |


#### Python API

```py
img_out = pillowfight.unpaper_blurfilter(img_in)
```


#### C API

```C
extern void pf_unpaper_blurfilter(const struct pf_bitmap *in, struct pf_bitmap *out);
```


#### Sources

* https://github.com/Flameeyes/unpaper
* https://github.com/Flameeyes/unpaper/blob/master/doc/basic-concepts.md


### Unpaper's Border

| Input | Output | Diff |
| ----- | ------ | ---- |
| [Black border problem 3](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem3.jpg) | [Border](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem3_border.jpg) | [Diff](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem3_border_diff.jpg) |


#### Python API

```py
img_out = pillowfight.unpaper_border(img_in)
```


#### C API

```C
extern void pf_unpaper_border(const struct pf_bitmap *in, struct pf_bitmap *out);
```


#### Sources

* https://github.com/Flameeyes/unpaper
* https://github.com/Flameeyes/unpaper/blob/master/doc/basic-concepts.md


### Unpaper's Grayfilter

| Input | Output | Diff |
| ----- | ------ | ---- |
| [Black border problem 3](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem.jpg) | [Filterd](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem_grayfilter.jpg) | [Diff](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem_grayfilter_diff.jpg) |


#### Python API

```py
img_out = pillowfight.unpaper_grayfilter(img_in)
```


#### C API

```C
extern void pf_unpaper_grayfilter(const struct pf_bitmap *in, struct pf_bitmap *out);
```


#### Sources

* https://github.com/Flameeyes/unpaper
* https://github.com/Flameeyes/unpaper/blob/master/doc/basic-concepts.md


### Unpaper's Masks

| Input | Output | Diff |
| ----- | ------ | ---- |
| [Black border problem 2](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem2.jpg) | [Masks](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem2_masks.jpg) | [Diff](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem2_masks_diff.jpg) |


#### Python API

```py
img_out = pillowfight.unpaper_masks(img_in)
```


#### C API

```C
extern void pf_unpaper_masks(const struct pf_bitmap *in, struct pf_bitmap *out);
```


#### Sources

* https://github.com/Flameeyes/unpaper
* https://github.com/Flameeyes/unpaper/blob/master/doc/basic-concepts.md


### Unpaper's Noisefilter

| Input | Output | Diff |
| ----- | ------ | ---- |
| [Black border problem](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem.jpg) | [Filtered](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem_noisefilter.jpg) | [Diff](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem_noisefilter_diff.jpg) |


#### Python API

```py
img_out = pillowfight.unpaper_noisefilter(img_in)
```


#### C API

```C
extern void pf_unpaper_noisefilter(const struct pf_bitmap *in, struct pf_bitmap *out);
```


#### Sources

* https://github.com/Flameeyes/unpaper
* https://github.com/Flameeyes/unpaper/blob/master/doc/basic-concepts.md
