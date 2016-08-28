import random

import PIL.Image


def transpose_axis(data, axis):
    shape = (
        len(data),
        len(data[0]),
        len(data[0][0])
    )

    new_shape = (
        shape[axis[0]],
        shape[axis[1]],
        shape[axis[2]],
    )
    axisinv = (
        axis.index(0),
        axis.index(1),
        axis.index(2)
    )

    new_position = [0, 0, 0]

    return [
        [
            [
                data \
                    [new_position[axisinv[0]]] \
                    [new_position[axisinv[1]]] \
                    [new_position[axisinv[2]]]
                for new_position[2] in range(0, new_shape[2])
            ]
            for new_position[1] in range(0, new_shape[1])
        ]
        for new_position[0] in range(0, new_shape[0])
    ]

    return out


def from_pil(pil_img):
    """
    Convert a Pillow image into an array of arrays of pixels (3 values)
    """
    pil_img = pil_img.convert("RGB")
    pil_bytes = pil_img.tobytes()
    (x, y) = pil_img.size

    assert(x * y * 3 == len(pil_bytes))

    output = [
        [
            [b for b in pil_bytes[offset:offset+3]]
            for offset in range(line_pos, line_pos + (y * 3), 3)
        ]
        for line_pos in range(0, x * y * 3, y * 3)
    ]

    return output


def to_pil(img_bytes):
    """
    Convert an array of arrays of pixels (3 values) into a Pillow image
    """
    (x, y) = (len(img_bytes), len(img_bytes[0]))

    data = b"".join(
        [
            b"".join(
                [bytes(pixel) for pixel in line]
            ) for line in img_bytes
        ]
    )
    return PIL.Image.frombytes(
        mode='RGB',
        size=(x, y),
        data=data,
    )


def create_random_pairs(nb_pairs, max_x, max_y):
    for _ in range(0, nb_pairs):
        yield (
            random.randint(0, max_x - 1),
            random.randint(0, max_y - 1),
        )
