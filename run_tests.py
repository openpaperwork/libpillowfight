#!/usr/bin/env python3
import unittest

from tests import tests_ace
from tests import tests_blackfilter

if __name__ == '__main__':
    unittest.main(tests_ace, verbosity=2, exit=False)
    unittest.main(tests_blackfilter, verbosity=2, exit=False)
