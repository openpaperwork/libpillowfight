import tempfile
import unittest

import PIL.Image

import pillowfight


class TestDiff(unittest.TestCase):
    def test_diff(self):
        with tempfile.NamedTemporaryFile(suffix='.jpg') as tmpfile:
            in_img = PIL.Image.open("tests/data/black_border_problem.jpg")
            in_img2 = PIL.Image.open(
                "tests/data/black_border_problem_blackfilter.jpg"
            )

            (has_diff, out_img) = pillowfight.diff(in_img, in_img2)
            in_img.close()

            self.assertTrue(has_diff)

            # beware of JPG compression
            out_img.save(tmpfile.name)
            out_img.close()
            out_img = PIL.Image.open(tmpfile.name)

        expected_img = PIL.Image.open(
            "tests/data/black_border_problem_diff.jpg"
        )
        self.assertEqual(out_img.tobytes(), expected_img.tobytes())
        expected_img.close()
