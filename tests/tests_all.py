import tempfile
import unittest

import PIL.Image

from pypillowfight import ace
from pypillowfight import blackfilter
from pypillowfight import blurfilter
from pypillowfight import border
from pypillowfight import grayfilter
from pypillowfight import masks
from pypillowfight import noisefilter


class TestAll(unittest.TestCase):
    def test_all(self):
        with tempfile.NamedTemporaryFile(suffix='.jpg') as tmpfile:
            in_img = PIL.Image.open("tests/data/black_border_problem.jpg")

            out_img = ace.ace(in_img, seed=0xDEADBEE)

            # unpaper order
            out_img = blackfilter.blackfilter(out_img)
            out_img = noisefilter.noisefilter(out_img)
            out_img = blurfilter.blurfilter(out_img)
            out_img = masks.masks(out_img)
            out_img = grayfilter.grayfilter(out_img)
            out_img = border.border(out_img)

            in_img.close()

            # beware of JPG compression
            out_img.save(tmpfile.name)
            out_img.close()
            out_img = PIL.Image.open(tmpfile.name)

        expected_img = PIL.Image.open(
            "tests/data/black_border_problem_all.jpg"
        )
        self.assertEqual(out_img.tobytes(), expected_img.tobytes())
        expected_img.close()
