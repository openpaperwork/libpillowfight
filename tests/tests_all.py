import tempfile
import unittest

import PIL.Image

import pypillowfight


class TestAll(unittest.TestCase):
    def test_all_1(self):
        with tempfile.NamedTemporaryFile(suffix='.jpg') as tmpfile:
            in_img = PIL.Image.open("tests/data/black_border_problem.jpg")

            out_img = pypillowfight.ace(in_img, seed=0xDEADBEE)

            # unpaper order
            out_img = pypillowfight.unpaper_blackfilter(out_img)
            out_img = pypillowfight.unpaper_noisefilter(out_img)
            out_img = pypillowfight.unpaper_blurfilter(out_img)
            out_img = pypillowfight.unpaper_masks(out_img)
            out_img = pypillowfight.unpaper_grayfilter(out_img)
            out_img = pypillowfight.unpaper_border(out_img)

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

    def test_all_2(self):
        with tempfile.NamedTemporaryFile(suffix='.jpg') as tmpfile:
            in_img = PIL.Image.open("tests/data/brightness_problem.jpg")

            out_img = pypillowfight.ace(in_img, seed=0xBEEDEAD)

            # unpaper order
            out_img = pypillowfight.unpaper_blackfilter(out_img)
            out_img = pypillowfight.unpaper_noisefilter(out_img)
            out_img = pypillowfight.unpaper_blurfilter(out_img)
            out_img = pypillowfight.unpaper_masks(out_img)
            out_img = pypillowfight.unpaper_grayfilter(out_img)
            out_img = pypillowfight.unpaper_border(out_img)

            in_img.close()

            # beware of JPG compression
            out_img.save(tmpfile.name)
            out_img.close()
            out_img = PIL.Image.open(tmpfile.name)

        expected_img = PIL.Image.open(
            "tests/data/brightness_problem_all.jpg"
        )
        self.assertEqual(out_img.tobytes(), expected_img.tobytes())
        expected_img.close()
