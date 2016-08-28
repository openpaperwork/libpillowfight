import sys
import unittest

import PIL.Image

sys.path = ["src"] + sys.path
from pypillowfight import ace


class TestACE(unittest.TestCase):
    def test_ace(self):
        img = PIL.Image.open("tests/data/brightness_problem.jpg")
        img = ace.ace(img)
        img.save("tests/data/brightness_problem_ace.jpg")


def get_all_tests():
    all_tests = unittest.TestSuite()

    test_names = [
        'test_ace',
    ]
    tests = unittest.TestSuite(map(TestACE, test_names))
    all_tests.addTest(tests)

    return all_tests
