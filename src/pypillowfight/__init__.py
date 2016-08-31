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


def blackfilter(img_in):
    img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.blackfilter(
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


def blurfilter(img_in):
    img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.blurfilter(
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


def border(img_in):
    img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.border(
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


def grayfilter(img_in):
    img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.grayfilter(
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


def masks(img_in):
    img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.masks(
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


def noisefilter(img_in):
    img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.noisefilter(
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
