import logging
import math
import random
import time

import PIL.Image

from .. import util

from . import _ace

logger = logging.getLogger(__name__)


def ace(img_in, slope=10, limit=1000, samples=5, seed=None):
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
        seed,
        img_out
    )
    return PIL.Image.frombytes(
        mode="RGBA",
        size=(img_in.size[0], img_in.size[1]),
        data=img_out
    )


def new_2d_mat(x, y):
    return [
        (y * [0.0])
        for a in range(0, x)
    ]


def create_rscore(x, y):
    return {
        "rmax": 0.0,
        "gmax": 0.0,
        "bmax": 0.0,
        "rmin": 2**32,
        "gmin": 2**32,
        "bmin": 2**32,
        "r": new_2d_mat(x, y),
        "g": new_2d_mat(x, y),
        "b": new_2d_mat(x, y),
    }


def calc_saturation(diff, slope, limit):
    ret = diff * slope
    if ret > limit:
        return limit
    if ret < -limit:
        return -limit
    return ret


def linear_scaling(v, in_max, in_min, out_max, out_min):
    slope = (out_max - out_min) / (in_max - in_min)
    return int(((v - in_min) * slope) + out_min)


def pure_py_ace(img, slope=10, limit=1000, samples=5, seed=None):
    random.seed(seed)

    img_size = img.size

    img = util.from_pil(img)
    img = util.transpose_axis(img, (2, 0, 1))

    img_r = img[0]
    img_g = img[1]
    img_b = img[2]

    samples = util.create_random_pairs(samples, img_size[0], img_size[1])
    samples = [x for x in samples]

    rscore = create_rscore(img_size[0], img_size[1])

    # Chromatic/Spatial Adjustment
    for i in range(0, img_size[1]):
        for j in range(0, img_size[0]):
            r_pixel = img_r[j][i]
            g_pixel = img_g[j][i]
            b_pixel = img_b[j][i]

            r_rscore_sum = 0.0
            g_rscore_sum = 0.0
            b_rscore_sum = 0.0

            denominator = 0.0

            # Calculate r score from whole image
            for (l, k) in samples:
                dist = math.sqrt(
                    ((j - l) ** 2) +
                    ((i - k) ** 2)
                )
                if dist < img_size[1] / 5:
                    continue
                r_rscore_sum += calc_saturation(
                    r_pixel - img_r[l][k], slope, limit
                ) / dist
                g_rscore_sum += calc_saturation(
                    g_pixel - img_g[l][k], slope, limit
                ) / dist
                b_rscore_sum += calc_saturation(
                    b_pixel - img_b[l][k], slope, limit
                ) / dist
                denominator += limit / dist

            r_rscore_sum /= denominator
            g_rscore_sum /= denominator
            b_rscore_sum /= denominator

            rscore['r'][j][i] = r_rscore_sum
            rscore['g'][j][i] = g_rscore_sum
            rscore['b'][j][i] = b_rscore_sum

            rscore['rmax'] = max(rscore['rmax'], r_rscore_sum)
            rscore['gmax'] = max(rscore['gmax'], g_rscore_sum)
            rscore['bmax'] = max(rscore['bmax'], b_rscore_sum)
            rscore['rmin'] = min(rscore['rmin'], r_rscore_sum)
            rscore['gmin'] = min(rscore['gmin'], g_rscore_sum)
            rscore['bmin'] = min(rscore['bmin'], b_rscore_sum)

    # Dynamic tone reproduction scaling
    for i in range(0, img_size[1]):
        for j in range(0, img_size[0]):
            img_r[j][i] = linear_scaling(
                rscore['r'][j][i],
                rscore['rmax'], rscore['rmin'],
                255.0, 0.0
            )
            img_g[j][i] = linear_scaling(
                rscore['g'][j][i],
                rscore['gmax'], rscore['gmin'],
                255.0, 0.0
            )
            img_b[j][i] = linear_scaling(
                rscore['b'][j][i],
                rscore['bmax'], rscore['bmin'],
                255.0, 0.0
            )

    img = util.transpose_axis(img, (1, 2, 0))
    img = util.to_pil(img)
    return img
