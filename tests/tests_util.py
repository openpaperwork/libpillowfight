import sys
import unittest

sys.path = ["src"] + sys.path
from pypillowfight import util

class TestUtil(unittest.TestCase):
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
        'test_transpose',
    ]
    tests = unittest.TestSuite(map(TestUtil, test_names))
    all_tests.addTest(tests)

    return all_tests
