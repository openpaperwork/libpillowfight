import PIL.Image

from . import _blurfilter


def blurfilter(img_in):
    img_in = img_in.convert("RGBA")  # Add alpha to align on 32bits
    img_out = bytes(img_in.size[0] * img_in.size[1] * 4 * [0])
    _blurfilter.blurfilter(
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
