import sys
import unittest

import PIL.Image

sys.path = ["src"] + sys.path
from pypillowfight import util

class TestUtil(unittest.TestCase):
    def test_frompil(self):
        in_img = PIL.Image.open("tests/data/brightness_problem.jpg")
        in_img = in_img.convert("RGB")
        imgbytes = util.from_pil(in_img)
        out_img = util.to_pil(imgbytes)
        self.assertEqual(in_img.tobytes(), out_img.tobytes())

    def test_transpose(self):
        ar_in = [
            [
                [0, 1, 2, 4],
                [5, 6, 7, 8],
            ],
            [
                [9, 10, 11, 12],
                [13, 14, 15, 16],
            ],
            [
                [17, 18, 19, 20],
                [21, 22, 23, 24]
            ]
        ]
        ar_out = util.transpose_axis(ar_in, (2, 0, 1))
        ar_out = util.transpose_axis(ar_out, (1, 2, 0))
        self.assertEqual(ar_in, ar_out)


def get_all_tests():
    all_tests = unittest.TestSuite()

    test_names = [
        'test_frompil',
        'test_transpose',
    ]
    tests = unittest.TestSuite(map(TestUtil, test_names))
    all_tests.addTest(tests)

    return all_tests
