import tempfile
import unittest

import PIL.Image

from pypillowfight import ace


class TestACE(unittest.TestCase):
    def test_ace(self):
        with tempfile.NamedTemporaryFile(suffix='.jpg') as tmpfile:
            in_img = PIL.Image.open("tests/data/brightness_problem.jpg")
            expected_img = PIL.Image.open("tests/data/brightness_problem_ace.jpg")

            out_img = ace.ace(in_img, seed=12345)
            in_img.close()

            # beware of JPG compression
            out_img.save(tmpfile.name)
            out_img.close()
            out_img = PIL.Image.open(tmpfile.name)

        self.assertEqual(out_img.tobytes(), expected_img.tobytes())
        expected_img.close()
