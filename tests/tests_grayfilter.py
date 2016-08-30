import tempfile
import unittest

import PIL.Image

from pypillowfight import grayfilter


class TestGrayFilter(unittest.TestCase):
    def test_grayfilter(self):
        with tempfile.NamedTemporaryFile(suffix='.jpg') as tmpfile:
            in_img = PIL.Image.open("tests/data/black_border_problem.jpg")
            out_img = grayfilter.grayfilter(in_img)
            in_img.close()

            # beware of JPG compression
            out_img.save(tmpfile.name)
            out_img.close()
            out_img = PIL.Image.open(tmpfile.name)

        expected_img = PIL.Image.open(
            "tests/data/black_border_problem_grayfilter.jpg"
        )
        self.assertEqual(out_img.tobytes(), expected_img.tobytes())
        expected_img.close()
