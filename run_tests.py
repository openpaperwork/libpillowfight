#!/usr/bin/env python3

import sys
import unittest

sys.path = ["src"] + sys.path
from tests import tests_ace
from tests import tests_util

if __name__ == '__main__':
    print("=== Util")
    unittest.TextTestRunner().run(tests_util.get_all_tests())
    print("=== Automatic Color Equalization")
    unittest.TextTestRunner().run(tests_ace.get_all_tests())
