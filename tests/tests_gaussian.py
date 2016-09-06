import tempfile
import unittest

import PIL.Image

import pillowfight


class TestGaussian(unittest.TestCase):
    def test_gaussian(self):
        with tempfile.NamedTemporaryFile(suffix='.jpg') as tmpfile:
            in_img = PIL.Image.open("tests/data/crappy_background.jpg")
            out_img = pillowfight.gaussian(in_img, sigma=20.0, nb_stddev=10)
            in_img.close()

            # beware of JPG compression
            out_img.save(tmpfile.name)
            out_img.close()
            out_img = PIL.Image.open(tmpfile.name)

        expected_img = PIL.Image.open(
            "tests/data/crappy_background_gaussian.jpg"
        )
        self.assertEqual(out_img.tobytes(), expected_img.tobytes())
        expected_img.close()
