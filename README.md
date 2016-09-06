# pillowfight

Library containing various image processing algorithms.
It includes Python 3 bindings designed to operate on Pillow images (PIL.Image).

The C library depends only on the libc.
The Python bindings depend only on Pillow.

APIs are designed to be as simple to use as possible. Default values are provided
for every parameters.

Python 2.x is *not* supported.

Available algorithms are listed below.


## Python API

The Python API can be compiled, installed and used without install the C library.

### Installation

```sh
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


### Gaussian

| Input | Output |
| ----- | ------ |
| [Crappy background](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/crappy_background.jpg) | [Gaussed](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/crappy_background_gaussian.jpg) |


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

| Input | Output |
| ----- | ------ |
| [Black border problen](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem.jpg) | [SWT](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/black_border_problem_swt.jpg) |
| [Crappy background](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/crappy_background.jpg) | [SWT](https://raw.githubusercontent.com/jflesch/libpillowfight/master/tests/data/crappy_background_swt.jpg) |


#### Python API

```py
img_out = pillowfight.swt(img_in)
```


#### C API

```C
extern void pf_swt(const struct pf_bitmap *in_img, struct pf_bitmap *out_img);
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
