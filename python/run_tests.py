import sys
import unittest
from pathlib import Path

def main():
    root = Path(__file__).resolve().parent
    sys.path.insert(0, str(root))
    
    loader = unittest.TestLoader()
    suite = loader.discover(start_dir=str(root / "tests"), pattern="test_*.py")
    
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)
    if not result.wasSuccessful():
        sys.exit(1)

if __name__ == "__main__":
    main()
