import logging
import multiprocessing
import time

import PIL.Image

from . import _clib


logger = logging.getLogger(__name__)


def ace(img_in, slope=10, limit=1000, samples=100, seed=None):
    if seed is None:
        seed = int(time.time())
    img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.ace(
        img_in.size[0],
        img_in.size[1],
        img_in.tobytes(),
        slope,
        limit,
        samples,
        multiprocessing.cpu_count(),
        seed,
        img_out
    )
    return PIL.Image.frombytes(
        mode="RGBA",
        size=(img_in.size[0], img_in.size[1]),
        data=img_out
    )


def unpaper_blackfilter(img_in):
    img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.unpaper_blackfilter(
        img_in.size[0],
        img_in.size[1],
        img_in.tobytes(),
        img_out
    )
    return PIL.Image.frombytes(
        mode="RGBA",
        size=(img_in.size[0], img_in.size[1]),
        data=img_out
    )
    return img_in


def unpaper_blurfilter(img_in):
    img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.unpaper_blurfilter(
        img_in.size[0],
        img_in.size[1],
        img_in.tobytes(),
        img_out
    )
    return PIL.Image.frombytes(
        mode="RGBA",
        size=(img_in.size[0], img_in.size[1]),
        data=img_out
    )
    return img_in


def unpaper_border(img_in):
    img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.unpaper_border(
        img_in.size[0],
        img_in.size[1],
        img_in.tobytes(),
        img_out
    )
    return PIL.Image.frombytes(
        mode="RGBA",
        size=(img_in.size[0], img_in.size[1]),
        data=img_out
    )
    return img_in


def canny(img_in):
    img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.canny(
        img_in.size[0],
        img_in.size[1],
        img_in.tobytes(),
        img_out
    )
    return PIL.Image.frombytes(
        mode="RGBA",
        size=(img_in.size[0], img_in.size[1]),
        data=img_out
    )
    return img_in


def gaussian(img_in, sigma=2.0, nb_stddev=5):
    img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.gaussian(
        img_in.size[0],
        img_in.size[1],
        img_in.tobytes(),
        img_out,
        sigma,
        nb_stddev
    )
    return PIL.Image.frombytes(
        mode="RGBA",
        size=(img_in.size[0], img_in.size[1]),
        data=img_out
    )
    return img_in


def unpaper_grayfilter(img_in):
    img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.unpaper_grayfilter(
        img_in.size[0],
        img_in.size[1],
        img_in.tobytes(),
        img_out
    )
    return PIL.Image.frombytes(
        mode="RGBA",
        size=(img_in.size[0], img_in.size[1]),
        data=img_out
    )
    return img_in


def unpaper_masks(img_in):
    img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.unpaper_masks(
        img_in.size[0],
        img_in.size[1],
        img_in.tobytes(),
        img_out
    )
    return PIL.Image.frombytes(
        mode="RGBA",
        size=(img_in.size[0], img_in.size[1]),
        data=img_out
    )
    return img_in


def unpaper_noisefilter(img_in):
    img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.unpaper_noisefilter(
        img_in.size[0],
        img_in.size[1],
        img_in.tobytes(),
        img_out
    )
    return PIL.Image.frombytes(
        mode="RGBA",
        size=(img_in.size[0], img_in.size[1]),
        data=img_out
    )
    return img_in


def sobel(img_in):
    img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.sobel(
        img_in.size[0],
        img_in.size[1],
        img_in.tobytes(),
        img_out
    )
    return PIL.Image.frombytes(
        mode="RGBA",
        size=(img_in.size[0], img_in.size[1]),
        data=img_out
    )
    return img_in
