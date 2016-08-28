import sys
import unittest

import PIL.Image

sys.path = ["src"] + sys.path
from pypillowfight import ace


class TestACE(unittest.TestCase):
    def test_ace(self):
        img = PIL.Image.open("tests/data/brightness_problem.jpg")
        img = ace.ace(img, seed=12345)
        img.save("tests/data/brightness_problem_ace.jpg")
