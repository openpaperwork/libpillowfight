import PIL.Image


def from_pil(pil_img):
    """
    Convert a Pillow image into an array of arrays of pixels (3 values)
    """
    pil_img = pil_img.convert("RGB")
    pil_bytes = pil_img.tobytes()
    (x, y) = pil_img.size

    output = []
    for line_bytes in range(0, y):
        line = []
        for offset in range(0, x):
            line.append(
                [b for b in pil_bytes[offset*3:(offset+1)*3]]
            )
        output.append(line)
    print (output[0][0])
    return output


def to_pil(img_bytes):
    """
    Convert an array of arrays of pixels (3 values) into a Pillow image
    """
    (x, y) = (len(img_bytes[0]), len(img_bytes))
    data = b"".join(
        [b"".join(
            [bytes(pixel) for pixel in line]
        ) for line in img_bytes]
    )
    return PIL.Image.frombytes(
        mode='RGB',
        size=(x, y),
        data=data,
    )


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
    out = []
    for new_position[0] in range(0, new_shape[0]):

        line = []
        for new_position[1] in range(0, new_shape[1]):

            val = []
            for new_position[2] in range(0, new_shape[2]):
                old_position = (
                    new_position[axisinv[0]],
                    new_position[axisinv[1]],
                    new_position[axisinv[2]]
                )
                val.append(
                    data[old_position[0]][old_position[1]][old_position[2]]
                )
            line.append(val)
        out.append(line)

    return out
