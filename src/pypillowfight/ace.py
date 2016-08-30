import logging
import multiprocessing
import time

import PIL.Image

from . import _ace


logger = logging.getLogger(__name__)


def ace(img_in, slope=10, limit=1000, samples=100, seed=None):
    if seed is None:
        seed = int(time.time())
    img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _ace.ace(
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
