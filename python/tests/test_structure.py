import sys
from pathlib import Path
import unittest

# Ensure python directory is in sys.path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

class TestPlatformStructure(unittest.TestCase):
    def test_package_imports(self):
        import chess_data
        import features
        import analytics
        import models
        import experiments
        import selfplay
        
        self.assertEqual(chess_data.__version__, "0.1.0")

if __name__ == '__main__':
    unittest.main()
