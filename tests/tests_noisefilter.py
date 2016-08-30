import tempfile
import unittest

import PIL.Image

from pypillowfight import noisefilter


class TestNoisefilter(unittest.TestCase):
    def test_noisefilter(self):
        with tempfile.NamedTemporaryFile(suffix='.jpg') as tmpfile:
            in_img = PIL.Image.open("tests/data/fold_problem.jpg")
            out_img = noisefilter.noisefilter(in_img)
            in_img.close()

            # beware of JPG compression
            out_img.save("tests/data/fold_problem_noisefilter2.jpg")
            out_img.save(tmpfile.name)
            out_img.close()
            out_img = PIL.Image.open(tmpfile.name)

        expected_img = PIL.Image.open(
            "tests/data/fold_problem_noisefilter.jpg"
        )
        self.assertEqual(out_img.tobytes(), expected_img.tobytes())
        expected_img.close()
