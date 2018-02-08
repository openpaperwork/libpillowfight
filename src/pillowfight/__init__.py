import logging
import multiprocessing
import time

import PIL.Image

from . import _clib


logger = logging.getLogger(__name__)


def ace(img_in, slope=10, limit=1000, samples=100, seed=None, nb_threads=None):
    """
    Automatic Color Equalization
    """
    if seed is None:
        seed = int(time.time())
    if img_in.mode != "RGBA":
        img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.ace(
        img_in.size[0],
        img_in.size[1],
        img_in.tobytes(),
        slope,
        limit,
        samples,
        nb_threads if nb_threads is not None else multiprocessing.cpu_count(),
        seed,
        img_out
    )
    # alpha channel is lost by algorithm anyway --> RGBA => RGB
    return PIL.Image.frombytes(
        mode="RGBA",
        size=(img_in.size[0], img_in.size[1]),
        data=img_out
    ).convert("RGB")


def compare(img_in, img_in2, tolerance=10):
    """
    Compare two images
    """
    if img_in.mode != "RGBA":
        img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    if img_in2.mode != "RGBA":
        img_in2 = img_in2.convert("RGBA")  # Add alpha to align on 32bits

    # img_out must have a size that is the smallest common denominator
    min_x = min(img_in.size[0], img_in2.size[0])
    min_y = min(img_in.size[1], img_in2.size[1])
    img_out = bytes(min_x * min_y * 4 * [0])
    out = _clib.compare(
        img_in.size[0],
        img_in.size[1],
        img_in2.size[0],
        img_in2.size[1],
        img_in.tobytes(),
        img_in2.tobytes(),
        img_out,
        tolerance
    )
    # alpha channel is lost by algorithm anyway --> RGBA => RGB
    return (out, PIL.Image.frombytes(
        mode="RGBA",
        size=(min_x, min_y),
        data=img_out
    ).convert("RGB"))


def unpaper_blackfilter(img_in):
    has_alpha = "A" in img_in.mode
    if img_in.mode != "RGBA":
        img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.unpaper_blackfilter(
        img_in.size[0],
        img_in.size[1],
        img_in.tobytes(),
        img_out
    )
    out = PIL.Image.frombytes(
        mode="RGBA",
        size=(img_in.size[0], img_in.size[1]),
        data=img_out
    )
    if not has_alpha:
        out = out.convert("RGB")
    return out


def unpaper_blurfilter(img_in):
    has_alpha = "A" in img_in.mode
    if img_in.mode != "RGBA":
        img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.unpaper_blurfilter(
        img_in.size[0],
        img_in.size[1],
        img_in.tobytes(),
        img_out
    )
    out = PIL.Image.frombytes(
        mode="RGBA",
        size=(img_in.size[0], img_in.size[1]),
        data=img_out
    )
    if not has_alpha:
        out = out.convert("RGB")
    return out


def unpaper_border(img_in):
    has_alpha = "A" in img_in.mode
    if img_in.mode != "RGBA":
        img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.unpaper_border(
        img_in.size[0],
        img_in.size[1],
        img_in.tobytes(),
        img_out
    )
    out = PIL.Image.frombytes(
        mode="RGBA",
        size=(img_in.size[0], img_in.size[1]),
        data=img_out
    )
    if not has_alpha:
        out = out.convert("RGB")
    return out


def canny(img_in):
    if img_in.mode != "RGBA":
        img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.canny(
        img_in.size[0],
        img_in.size[1],
        img_in.tobytes(),
        img_out
    )
    # alpha channel is lost by algorithm anyway --> RGBA => RGB
    return PIL.Image.frombytes(
        mode="RGBA",
        size=(img_in.size[0], img_in.size[1]),
        data=img_out
    ).convert("RGB")


def gaussian(img_in, sigma=2.0, nb_stddev=5):
    if img_in.mode != "RGBA":
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
    # alpha channel is lost by algorithm anyway --> RGBA => RGB
    return PIL.Image.frombytes(
        mode="RGBA",
        size=(img_in.size[0], img_in.size[1]),
        data=img_out
    ).convert("RGB")


def unpaper_grayfilter(img_in):
    has_alpha = "A" in img_in.mode
    if img_in.mode != "RGBA":
        img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.unpaper_grayfilter(
        img_in.size[0],
        img_in.size[1],
        img_in.tobytes(),
        img_out
    )
    out = PIL.Image.frombytes(
        mode="RGBA",
        size=(img_in.size[0], img_in.size[1]),
        data=img_out
    )
    if not has_alpha:
        out = out.convert("RGB")
    return out


def unpaper_masks(img_in):
    has_alpha = "A" in img_in.mode
    if img_in.mode != "RGBA":
        img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.unpaper_masks(
        img_in.size[0],
        img_in.size[1],
        img_in.tobytes(),
        img_out
    )
    out = PIL.Image.frombytes(
        mode="RGBA",
        size=(img_in.size[0], img_in.size[1]),
        data=img_out
    )
    if not has_alpha:
        out = out.convert("RGB")
    return out


def unpaper_noisefilter(img_in):
    has_alpha = "A" in img_in.mode
    if img_in.mode != "RGBA":
        img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.unpaper_noisefilter(
        img_in.size[0],
        img_in.size[1],
        img_in.tobytes(),
        img_out
    )
    out = PIL.Image.frombytes(
        mode="RGBA",
        size=(img_in.size[0], img_in.size[1]),
        data=img_out
    )
    if not has_alpha:
        out = out.convert("RGB")
    return out


def sobel(img_in):
    if img_in.mode != "RGBA":
        img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.sobel(
        img_in.size[0],
        img_in.size[1],
        img_in.tobytes(),
        img_out
    )
    # alpha channel is lost by algorithm anyway --> RGBA => RGB
    return PIL.Image.frombytes(
        mode="RGBA",
        size=(img_in.size[0], img_in.size[1]),
        data=img_out
    ).convert("RGB")


SWT_OUTPUT_BW_TEXT = 0
SWT_OUTPUT_GRAYSCALE_TEXT = 1
SWT_OUTPUT_ORIGINAL_BOXES = 2


def swt(img_in, output_type=SWT_OUTPUT_BW_TEXT):
    if img_in.mode != "RGBA":
        img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _clib.swt(
        img_in.size[0],
        img_in.size[1],
        img_in.tobytes(),
        img_out,
        output_type
    )
    # alpha channel is lost by algorithm anyway --> RGBA => RGB
    return PIL.Image.frombytes(
        mode="RGBA",
        size=(img_in.size[0], img_in.size[1]),
        data=img_out
    ).convert("RGB")


def get_version():
    return _clib.get_version()


__version__ = get_version()
