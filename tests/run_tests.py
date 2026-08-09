#!/usr/bin/env python3

import sys
import unittest
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(PROJECT_ROOT))


def main() -> int:
    """Discover the unit tests and return a shell-friendly exit status."""
    start_dir = Path(__file__).resolve().parent / "unit"
    suite = unittest.defaultTestLoader.discover(str(start_dir), pattern="test*.py")
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    raise SystemExit(main())
