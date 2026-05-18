"""Compatibility entry point for the mapper unittest package."""

from pathlib import Path


def load_tests(loader, tests, pattern):
    """Load mapper tests for ``python -m unittest nes_py.tests.test_mappers``."""
    del tests
    start_dir = Path(__file__).with_name('mappers')
    top_level_dir = Path(__file__).parents[2]
    return loader.discover(
        str(start_dir),
        pattern=pattern or 'test*.py',
        top_level_dir=str(top_level_dir),
    )
