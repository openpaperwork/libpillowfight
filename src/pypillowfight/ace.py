from . import util


def ace(img, slope=10, limit=1000, samples=500):
    img = util.from_pil(img)

    img = util.transpose_axis(img, (2, 0, 1))

    img = util.transpose_axis(img, (1, 2, 0))

    img = util.to_pil(img)
    return img
