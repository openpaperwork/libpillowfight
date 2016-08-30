#!/usr/bin/env python3
import unittest

from tests import tests_ace
from tests import tests_blackfilter
from tests import tests_blurfilter
from tests import tests_border
from tests import tests_grayfilter
from tests import tests_masks
from tests import tests_noisefilter

if __name__ == '__main__':
    unittest.main(tests_ace, verbosity=2, exit=False)
    unittest.main(tests_blackfilter, verbosity=2, exit=False)
    unittest.main(tests_blurfilter, verbosity=2, exit=False)
    unittest.main(tests_border, verbosity=2, exit=False)
    unittest.main(tests_grayfilter, verbosity=2, exit=False)
    unittest.main(tests_masks, verbosity=2, exit=False)
    unittest.main(tests_noisefilter, verbosity=2, exit=False)
