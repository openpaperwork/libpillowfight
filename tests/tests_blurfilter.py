import tempfile
import unittest

import PIL.Image

import pypillowfight


class TestBlurfilter(unittest.TestCase):
    def test_blurfilter(self):
        with tempfile.NamedTemporaryFile(suffix='.jpg') as tmpfile:
            in_img = PIL.Image.open("tests/data/black_border_problem.jpg")
            out_img = pypillowfight.unpaper_blurfilter(in_img)
            in_img.close()

            # beware of JPG compression
            out_img.save(tmpfile.name)
            out_img.close()
            out_img = PIL.Image.open(tmpfile.name)

        expected_img = PIL.Image.open(
            "tests/data/black_border_problem_blurfilter.jpg"
        )
        self.assertEqual(out_img.tobytes(), expected_img.tobytes())
        expected_img.close()
