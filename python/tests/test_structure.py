import importlib
import unittest

PACKAGES = ["chess_data", "features", "analytics", "models", "experiments", "selfplay"]


class TestPlatformStructure(unittest.TestCase):
    def test_package_imports(self):
        for name in PACKAGES:
            self.assertIsNotNone(importlib.import_module(name), name)
        self.assertEqual(importlib.import_module("chess_data").__version__, "0.1.0")


if __name__ == "__main__":
    unittest.main()
